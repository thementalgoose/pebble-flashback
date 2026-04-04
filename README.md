<h1 align="center">Flashback for Pebble</h1>

<p align="center">
  <a href="https://github.com/thementalgoose/pebble-flashback/releases"><img src="https://img.shields.io/github/v/release/thementalgoose/pebble-flashback"/></a>
  <a href="https://apps.rebble.io/en_US/application/6929da3470731e00092debea"><img src="https://img.shields.io/static/v1?label=Rebble%20App%20Listing&message=%20&logoColor=white&color=833FD6&style=flat"/></a>
  <a href="https://dev-portal.rebble.io/"><img src="https://img.shields.io/static/v1?label=Rebble%20Dev%20Portal&message=%20&logoColor=white&color=B84E2E&style=flat"/></a>
</p>

| Codename | Pebble name       | Menu | Calendar | Drivers |
|----------|-------------------|------|----------|---------|
| basalt   | Pebble Time       | <img src="appstore/basalt/menu.png" width="120"/>  | <img src="appstore/basalt/race.png" width="120"/>  | <img src="appstore/basalt/drivers.png" width="120"/>  |
| chalk    | Pebble Time Round | <img src="appstore/chalk/menu.png" width="120"/>   | <img src="appstore/chalk/race.png" width="120"/>   | <img src="appstore/chalk/drivers.png" width="120"/>   |
| diorite  | Pebble 2          | <img src="appstore/diorite/menu.png" width="120"/> | <img src="appstore/diorite/race.png" width="120"/> | <img src="appstore/diorite/drivers.png" width="120"/> |
| emery    | Pebble 2 Duo      | <img src="appstore/emery/menu.png" width="144"/>   | <img src="appstore/emery/race.png" width="144"/>   | <img src="appstore/emery/drivers.png" width="144"/>   |
| flint    | Pebble Time 2     | <img src="appstore/flint/menu.png" width="120"/>   | <img src="appstore/flint/race.png" width="120"/>   | <img src="appstore/flint/drivers.png" width="120"/>   |
| gabbro   | Pebble Round 2    | <img src="appstore/gabbro/menu.png" width="120"/>  | <img src="appstore/gabbro/race.png" width="120"/>  | <img src="appstore/gabbro/drivers.png" width="120"/>  |

Pebble watch app for the Flashback API 🎉

### Currently supported

- [x] Calendar view (with races split by Upcoming and Previous)
- [x] Race schedule view per race
- [x] Drivers standings
- [x] Teams standings

#### Building

- Python 3.10.x required

```bash
# Booting the emulator
pebble install --emulator flint

# Building
pebble build

# Running
pebble install
pebble install --cloudpebble 
pebble install --emulator flint --logs
```

#### Useful Links

- [Hardware information](https://developer.rebble.io/guides/tools-and-resources/hardware-information/)
- [UI Samples](https://github.com/pebble-examples/ui-patterns/)
- [Modular Architecture](https://github.com/pebble-examples/modular-app-example/blob/master/src/windows/main_window.h)
- [Best Practices](https://developer.rebble.io/guides/best-practices/modular-app-architecture/)
