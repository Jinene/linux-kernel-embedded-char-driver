# Embedded Linux Character Device Driver

## Overview
This project implements a production-style Linux kernel character driver featuring:

- Dynamic device registration
- Interrupt handling
- Kernel threading
- Sysfs integration
- IOCTL interface
- Concurrency control (mutex + spinlock)
- User-space test application

## Architecture
User Space → Character Device → Kernel Driver → Interrupt → Kernel Thread → Sysfs

## Build

### Kernel Module
cd driver
make

### Load
sudo ./scripts/load.sh

### Test
cd user_app
make
./test_app

## Remove
sudo ./scripts/unload.sh
