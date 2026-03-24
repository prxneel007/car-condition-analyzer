# 🚖 Dynamic Ride-Sharing System

A fully-featured ride-sharing simulation built in **C** with a **GTK3** GUI. Inspired by the design aesthetic of Uber/Ola (dark theme, green accents), this desktop app simulates the core mechanics of a ride-hailing platform — from driver registration to fare-based ride matching.

---

## Features

- **Driver management** — add/update drivers, toggle online/offline status, track earnings
- **Rider management** — register riders with start and destination coordinates
- **Nearest-driver matching** — matches a rider to the closest available online driver using a **min-heap** and **Euclidean distance**
- **Full ride lifecycle** — match → finish (driver moves to destination, earnings updated) → cancel
- **Location updates** — update any driver or rider's position at runtime
- **Earnings dashboard** — view total earnings per driver
- **Action logs** — detailed log panel for every operation performed
- **Auto-clear inputs** — input fields reset after each operation for a clean UX
- **Custom dark theme** — styled with `#0F141A` background and `#00C26E` accent (Uber/Ola vibe)

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C (C11) |
| GUI Framework | GTK3 |
| Data Structures | Min-Heap, Hash Tables (open chaining) |
| Math | Euclidean distance (`<math.h>`) |
| Build | GCC + pkg-config |

---

## Data Structures Used

### Hash Tables
- Separate hash tables for drivers and riders, each with prime capacity (101 buckets) to minimise collisions
- Enables **O(1) average-case** lookup, insert, and update for both entity types

### Min-Heap
- Used for the driver-matching algorithm
- Drivers are inserted by distance from the rider's start coordinates
- The root always holds the nearest available online driver — **O(log n)** matching

---

## Project Structure

```
rideshare/
├── rideshare.c     # Full source — UI, data structures, ride logic
└── README.md
```

---

## How to Compile & Run

### Prerequisites

**Ubuntu / Debian:**
```bash
sudo apt install libgtk-3-dev
```

**Fedora:**
```bash
sudo dnf install gtk3-devel
```

### Compile

```bash
gcc rideshare.c -o rideshare $(pkg-config --cflags --libs gtk+-3.0) -lm
```

### Run

```bash
./rideshare
```

---

## How It Works

### Adding a Driver
Enter a driver ID and coordinates (x, y), then click **Add Driver**. The driver is stored in the hash table and marked as online and available.

### Requesting a Ride
Enter a rider ID with start and destination coordinates. Click **Match Rider** — the system scans all online, available drivers, builds a min-heap by distance, and assigns the nearest one. A fare is calculated based on distance.

### Completing a Ride
Click **Finish Ride** for a matched rider. The driver moves to the destination coordinates, the fare is added to their earnings, and they become available again.

### Cancelling a Ride
Unmatches the rider and frees the driver back to available status.

---

## What I Learned

- Implementing a **min-heap from scratch** in C for priority-based selection
- Designing **hash tables with prime-capacity buckets** to reduce collision rates
- Modelling a real-world system (ride-hailing) using pointer-based C structs
- Building a multi-panel GTK3 interface with a log view and form inputs
- Applying **Euclidean distance** for geolocation proximity calculations

---

## Author

**Chirra Yuktha Praneel**  
B.Tech CSE (AI & ML) — PES University, Bangalore  
[LinkedIn](https://www.linkedin.com/in/yuktha-praneel-chirra-141b153a2)
