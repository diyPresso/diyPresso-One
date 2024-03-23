# TODO

Settings/display show negative decimal numbers correctly

When brewing: Always show main menu

When brewing: Show tared weight

Show level of reservoir a (8level) bar on main screen

Show error screens in case of error

Implement tetris screensaver....

# Findings diyP-Controller PBrier 20240322 – V1.3.0

## PID is nog niet goed / Temp overshoot van 2 graden
* PID tunen (Bernard)
* Default PID:  5 / 0.1 / 10
* Andere controller?

## Kan niet pompen in heatup
* Mogelijk maken

## Klant weet niet dat machine in ready state is
* In display laten zien?
* Time niet laten zien in idle stand, toestand boiler laten zien

## Power FF is nul / Feed forward lijkt niks te doen, ook niet op 25%
* Geen idee

## Kan gewoon koffie door blijven zetten als gewicht onder “tar weight”komt (negatief)
* Beveiliging inbouwen

## Na ongeveer tien miunten (kan ook langer of korter zijn) gaat de temp naar 0 deg
* Dit lijkt je beveiliging te zijn (#define TIMEOUT_HEATING (1000*600) // maximum heater on time: 10 minutes)
Is opgelost?

