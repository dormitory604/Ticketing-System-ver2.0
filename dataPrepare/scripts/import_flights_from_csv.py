#!/usr/bin/env python3
"""Utility script to normalize the Chinese flight CSV into SQLite.

Steps performed:
1. Read the CSV (default: sourceData/data.csv).
2. Expand weekly schedules into concrete departures between the provided date window
   (default window: 2025-10-01 through 2025-12-31).
3. Estimate seat counts/prices so the rows match the server's Flight schema.
4. Truncate Flight/Booking to avoid duplicated data, then bulk insert.

Usage example:
    python scripts/import_flights_from_csv.py \
        --csv sourceData/data.csv \
        --db ~/Documents/flight_system.db
"""

from __future__ import annotations

import argparse
import csv
import random
import sqlite3
from dataclasses import dataclass
from datetime import date, datetime, time, timedelta, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

DAY_COLUMNS: Sequence[tuple[str, int]] = (
    ("周一班期", 0),
    ("周二班期", 1),
    ("周三班期", 2),
    ("周四班期", 3),
    ("周五班期", 4),
    ("周六班期", 5),
    ("周日班期", 6),
)

SEAT_RULES: Sequence[tuple[str, int]] = (
    ("空客321", 210),
    ("空客320", 186),
    ("空客319", 160),
    ("波音787", 240),
    ("波音777", 250),
    ("波音757", 200),
    ("波音747", 366),
    ("波音737", 168),
    ("ERJ-190", 104),
    ("空客321neo", 220),
    ("空客320neo", 190),
    ("CRJ", 86),
    ("其他机型", 150),
    ("JET", 150),
)

DEFAULT_SEATS = 170
PRICE_NOISE_STD_RATIO = 0.2  # larger std-dev so fares wiggle more
PRICE_NOISE_CLAMP_RATIO = 0.5  # allow up to +/-50% swing around baseline
PRICE_NOISE_MIN_ABS = 35.0  # guarantee noticeable bumps even on short legs
DEFAULT_DEMO_USERS = 25
DEFAULT_BOOKING_COUNT = 100

FLIGHT_SCHEMA = """
CREATE TABLE IF NOT EXISTS Flight (
    flight_id INTEGER PRIMARY KEY AUTOINCREMENT,
    flight_number TEXT NOT NULL,
    model TEXT,
    origin TEXT NOT NULL,
    destination TEXT NOT NULL,
    departure_time DATETIME NOT NULL,
    arrival_time DATETIME NOT NULL,
    total_seats INTEGER NOT NULL,
    remaining_seats INTEGER NOT NULL,
    price REAL NOT NULL,
    is_deleted INTEGER NOT NULL DEFAULT 0
);
"""

USER_SCHEMA = """
CREATE TABLE IF NOT EXISTS User (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    is_admin INTEGER NOT NULL DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
"""

BOOKING_SCHEMA = """
CREATE TABLE IF NOT EXISTS Booking (
    booking_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    flight_id INTEGER NOT NULL,
    booking_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT NOT NULL,
    FOREIGN KEY (user_id) REFERENCES User (user_id),
    FOREIGN KEY (flight_id) REFERENCES Flight (flight_id)
);
"""


@dataclass
class FlightRecord:
    flight_number: str
    model: str
    origin: str
    destination: str
    departure_time: str  # YYYY-MM-DD HH:MM:SS
    arrival_time: str
    total_seats: int
    remaining_seats: int
    price: int
    is_deleted: int = 0

    def as_tuple(self) -> tuple:
        return (
            self.flight_number,
            self.model,
            self.origin,
            self.destination,
            self.departure_time,
            self.arrival_time,
            self.total_seats,
            self.remaining_seats,
            self.price,
            self.is_deleted,
        )


def parse_time(value: str) -> time | None:
    value = (value or "").strip()
    if not value:
        return None

    fmt_candidates = ["%H:%M:%S", "%H:%M"]
    for fmt in fmt_candidates:
        try:
            return datetime.strptime(value, fmt).time()
        except ValueError:
            continue
    return None


def parse_distance_km(raw: str) -> float:
    raw = (raw or "").strip().replace(",", "")
    if not raw:
        return 0.0
    try:
        return float(raw)
    except ValueError:
        return 0.0


def estimate_price(distance_km: float) -> int:
    """Very rough price estimation based on distance."""
    if distance_km <= 0:
        distance_km = 800.0
    base = 120.0
    per_km = 0.82 if distance_km > 1500 else 0.95
    price = base + distance_km * per_km

    # Add controlled Gaussian noise so prices don't look perfectly linear.
    sigma = max(price * PRICE_NOISE_STD_RATIO, PRICE_NOISE_MIN_ABS)
    noisy_price = random.gauss(price, sigma)
    lower_bound = max(200.0, price * (1 - PRICE_NOISE_CLAMP_RATIO))
    upper_bound = price * (1 + PRICE_NOISE_CLAMP_RATIO)
    bounded_price = min(max(noisy_price, lower_bound), upper_bound)
    return int(round(bounded_price))


def estimate_seats(model: str) -> int:
    model = (model or "").upper()
    for keyword, seats in SEAT_RULES:
        if keyword.upper() in model:
            return seats
    return DEFAULT_SEATS


def collect_active_weekdays(row: Dict[str, str]) -> List[int]:
    active = []
    for column, weekday in DAY_COLUMNS:
        value = row.get(column, "")
        if "有班期" in value:
            active.append(weekday)
    return active


def daterange(start: date, end: date) -> Iterable[date]:
    current = start
    delta = timedelta(days=1)
    while current <= end:
        yield current
        current += delta


def expand_row_to_flights(
    row: Dict[str, str],
    window_start: date,
    window_end: date,
) -> List[FlightRecord]:
    active_weekdays = set(collect_active_weekdays(row))
    if not active_weekdays:
        return []

    flight_number = row.get("航班班次", "").strip()
    model = row.get("机型", "").strip() or "UNKNOWN"
    origin = row.get("出发城市", "").strip()
    destination = row.get("到达城市", "").strip()
    dep_time = parse_time(row.get("起飞时间", ""))
    arr_time = parse_time(row.get("降落时间", ""))

    if not (flight_number and origin and destination and dep_time and arr_time):
        return []

    distance = parse_distance_km(row.get("里程（公里）", ""))
    seats = estimate_seats(model)

    records: List[FlightRecord] = []
    for current_date in daterange(window_start, window_end):
        if current_date.weekday() not in active_weekdays:
            continue

        departure_dt = datetime.combine(current_date, dep_time)
        arrival_dt = datetime.combine(current_date, arr_time)
        if arrival_dt <= departure_dt:
            arrival_dt += timedelta(days=1)

        remaining = random.randint(0, seats)
        price = estimate_price(distance)

        record = FlightRecord(
            flight_number=flight_number,
            model=model,
            origin=origin,
            destination=destination,
            departure_time=departure_dt.strftime("%Y-%m-%d %H:%M:%S"),
            arrival_time=arrival_dt.strftime("%Y-%m-%d %H:%M:%S"),
            total_seats=seats,
            remaining_seats=remaining,
            price=price,
        )
        records.append(record)

    return records


def load_csv_rows(csv_path: Path) -> List[Dict[str, str]]:
    with csv_path.open("r", encoding="utf-8-sig", newline="") as fh:
        reader = csv.DictReader(fh)
        return [row for row in reader]


def ensure_is_deleted_column(conn: sqlite3.Connection) -> None:
    exists = conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='Flight';"
    ).fetchone()
    if not exists:
        return

    columns = conn.execute("PRAGMA table_info(Flight);").fetchall()
    if not any(col[1] == "is_deleted" for col in columns):
        conn.execute("ALTER TABLE Flight ADD COLUMN is_deleted INTEGER NOT NULL DEFAULT 0;")


def ensure_core_tables(conn: sqlite3.Connection) -> None:
    conn.execute(USER_SCHEMA)
    conn.execute(FLIGHT_SCHEMA)
    conn.execute(BOOKING_SCHEMA)
    ensure_is_deleted_column(conn)


def ensure_demo_users(conn: sqlite3.Connection, target_count: int = DEFAULT_DEMO_USERS) -> List[int]:
    rows = conn.execute("SELECT user_id FROM User ORDER BY user_id;").fetchall()
    existing_count = len(rows)
    to_create = max(0, target_count - existing_count)
    if to_create:
        payload = []
        for idx in range(existing_count + 1, existing_count + to_create + 1):
            username = f"demo_user_{idx:03d}"
            password = f"demo_pass_{random.randint(1000, 9999)}"
            payload.append((username, password, 0))
        conn.executemany(
            "INSERT OR IGNORE INTO User (username, password, is_admin) VALUES (?, ?, ?)",
            payload,
        )
        rows = conn.execute("SELECT user_id FROM User ORDER BY user_id;").fetchall()
    return [row[0] for row in rows]


def seed_bookings(
    conn: sqlite3.Connection,
    flight_ids: Sequence[int],
    user_ids: Sequence[int],
    count: int = DEFAULT_BOOKING_COUNT,
) -> None:
    if count <= 0 or not flight_ids or not user_ids:
        return

    statuses = ["CONFIRMED", "CANCELLED", "PENDING"]
    weights = [0.7, 0.15, 0.15]
    bookings: List[tuple[int, int, str, str]] = []
    for _ in range(count):
        user_id = random.choice(user_ids)
        flight_id = random.choice(flight_ids)
        days_ago = random.randint(0, 60)
        minutes_offset = random.randint(0, 23 * 60)
        booking_time = (
            datetime.now(timezone.utc) - timedelta(days=days_ago, minutes=minutes_offset)
        ).strftime("%Y-%m-%d %H:%M:%S")
        status = random.choices(statuses, weights=weights, k=1)[0]
        bookings.append((user_id, flight_id, booking_time, status))

    conn.executemany(
        "INSERT INTO Booking (user_id, flight_id, booking_time, status) VALUES (?, ?, ?, ?)",
        bookings,
    )


def insert_flights(db_path: Path, flights: Sequence[FlightRecord]) -> None:
    if not flights:
        print("No flights to insert; aborting.")
        return

    conn = sqlite3.connect(db_path)
    try:
        conn.execute("PRAGMA foreign_keys = ON;")
        ensure_core_tables(conn)
        with conn:
            conn.execute("DELETE FROM Booking;")
            conn.execute("DELETE FROM Flight;")
            conn.executemany(
                """
                INSERT INTO Flight (
                    flight_number, model, origin, destination,
                    departure_time, arrival_time,
                    total_seats, remaining_seats, price, is_deleted
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                [record.as_tuple() for record in flights],
            )

            user_ids = ensure_demo_users(conn)
            flight_rows = conn.execute("SELECT flight_id FROM Flight;").fetchall()
            flight_ids = [row[0] for row in flight_rows]
            seed_bookings(conn, flight_ids, user_ids)
    finally:
        conn.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="Import cleaned flights into SQLite.")
    parser.add_argument("--csv", type=Path, default=Path("sourceData/data.csv"))
    parser.add_argument(
        "--db",
        type=Path,
        default=Path.home() / "Documents" / "flight_system.db",
        help="Destination SQLite database (Flight table will be truncated).",
    )
    parser.add_argument("--start", default="2025-10-01", help="Inclusive start date (YYYY-MM-DD)")
    parser.add_argument("--end", default="2025-12-31", help="Inclusive end date (YYYY-MM-DD)")
    parser.add_argument("--seed", type=int, default=None, help="Optional RNG seed for reproducible remaining seats")

    args = parser.parse_args()

    window_start = datetime.strptime(args.start, "%Y-%m-%d").date()
    window_end = datetime.strptime(args.end, "%Y-%m-%d").date()
    if window_end < window_start:
        raise SystemExit("End date must be after start date.")

    if not args.csv.exists():
        raise SystemExit(f"CSV file not found: {args.csv}")

    if args.seed is not None:
        random.seed(args.seed)

    rows = load_csv_rows(args.csv)
    flights: List[FlightRecord] = []
    for row in rows:
        flights.extend(expand_row_to_flights(row, window_start, window_end))

    insert_flights(args.db, flights)

    print(
        f"Imported {len(flights)} flights from {args.csv} "
        f"into {args.db} for window {args.start}~{args.end}."
    )


if __name__ == "__main__":
    main()
