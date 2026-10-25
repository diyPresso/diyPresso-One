# diyPresso One & Two: menu structure and settings

Text for the Quick Installation & User Guide, firmware 2.0.0 (currently 2.0.0-B2).

This text replaces the sections "Menu structure and instructions" (last paragraph) and "Other settings" of:

- diyPresso One quick installation & user guide V1.1.3
- diyPresso Two quick installation & user guide V1.1.2

It is written for both guides. Parts marked **[diyPresso Two only]** are only for the diyPresso Two guide: leave them out of the One guide. The menu of the One simply does not show these items.

## What changes compared to the current guides

- The menu is no longer one long list of 20 (One) or 25 (Two) pages. It now has a main menu with submenus, and shows 3 items at a time.
- There is no `<SAVE>` page anymore. When you leave the menu after changing something, the machine asks "Save changes?".
- Removed from the menu: FF-heat Value (the feed forward is now one setting, "Feedforward"), Extraction weight, Zero Counter (now done from the Shot counter), Commissioning status (now "Commissioning" in System), `<EXIT>` and `<SAVE>`.
- New in the menu: Knob direction, Status, Firmware, Hardware, and the advanced settings in Boiler PID and Steam PID.
- Tare, resetting the shot counter, commissioning and reset to defaults now ask "Are you sure?" first.
- New: each submenu ends with an item to set only the settings of that submenu back to their defaults ("Set brew defs", "Set steam defs", "Set reservoir defs", "Set boiler PID defs", "Set steam PID defs").
- Main screen: turning the button no longer opens the 'machine status' page. This page is now in the menu: System > Status.
  Remove the sentence "Turning the (right) button in the main screen will bring you to the 'machine status' page."
- Main screen: weight and water level indicator are one position further to the right. The screen pictures can stay as they are.
- The start-up screen now only shows the diyPresso logo (no version, date and time).
- The screen pictures of the settings menu ("SETTINGS [ 1/25] Temperature" and "<SAVE>") must be replaced. The screens below can be used for new pictures. On the display, `->` is a small arrow and `°` is the degree sign.

---

# Text for the guide

## Settings menu

Push the button right of the screen to enter the menu. The main menu shows the submenus. On the top right, you can see your position in the menu.

```
SETTINGS         1/7
>Brew             ->
 Steam            ->
 Reservoir        ->
```

Turn the button to move the cursor (`>`) and push the button to open a submenu. In a submenu, each line shows a setting and its current value.

```
BREW             1/6
>Temperature  98.0°C
 Pre-infusion   3.0s
 Infusion       1.0s
```

To change a setting, move the cursor to it and push the button. The screen shows MODIFY. Turn the button to change the value and push the button again to confirm.

```
MODIFY          BREW
Temperature
  98.5 °C
[TURN]  [PRESS] = OK
```

Choose "< Back" at the end of a submenu to go back to the main menu. Choose "< Exit" at the end of the main menu to leave the menu. If you have changed a setting, the screen asks if you want to save your changes. Turn the button to YES and push the button to save. NO is selected first: push the button on NO to leave the menu without saving.

```
SETTINGS
Save changes?

   [NO]   YES
```

Your changes are only used after saving. If you start a shot while you are in the menu, the menu closes and unsaved changes are lost.

Some items carry out an action, like taring the reservoir. These ask "Are you sure?" first. NO is selected first: turn the button to YES and push the button to carry out the action. An action is saved immediately.

The last item of each submenu (before "< Back") sets the settings of that submenu back to their defaults, for example "Set brew defs" in Brew. The screen asks for confirmation; NO is selected first. Like a changed setting, this is only saved when you leave the menu and choose YES at "Save changes?". The tare of the reservoir and the shot counter are not changed.

```
BREW
Set brew defs?

   [NO]   YES
```

The menu has these submenus:

| Submenu    | Contents                                                      |
|------------|---------------------------------------------------------------|
| Brew       | brew settings and shot counter                                |
| Steam      | steam settings **[diyPresso Two only]**                                 |
| Reservoir  | tare and weight trim of the reservoir                         |
| Boiler PID | temperature controller of the boiler (advanced)               |
| Steam PID  | temperature controller of the steamer (advanced) **[diyPresso Two only]** |
| System     | status, WiFi, knob direction, commissioning, reset and version |

## Brew

1. **Temperature:** This is the value for the temperature setpoint of the boiler. Default is 98.0°C.
2. **Pre-infusion:** This is the pre-infusion time, the period that the pump is on to wet the puck. Default is 3.0 s.
3. **Infusion:** Infusion time indicates the time the pump is off and the puck can settle. Default is 1.0 s.
4. **Extraction:** Extraction time or shot time is the time the pump is on again after pre-infusion and infusion, actually extracting the espresso. Default is 25.0 s.
5. **Shot counter:** Counts the number of shots that have been made. Push the button to reset the counter to 0. The screen shows the current count and asks "Reset counter?".

   ```
   Shot counter    1234
   Reset counter?

      [NO]   YES
   ```
6. **Set brew defs:** Sets settings 1-4 back to their defaults. The shot counter is not changed.

## Steam [diyPresso Two only]

1. **Temperature:** This is the value for the temperature setpoint of the thermoblock. Default is 165.0°C.
2. **Pump power:** Adjust the power for the steam pump. Standard is 100%.
3. **Set steam defs:** Sets settings 1-2 back to their defaults.

## Reservoir

1. **Tare (full):** Here you can tare the balance of the reservoir. Fill up the reservoir, looking on the side of the diyPresso so you can make sure not to overflow the reservoir. When the reservoir is full, push the button on this item and confirm with YES. The diyPresso will know the reservoir is tared and will after this stop automatically when the water runs out.
2. **Weight trim:** Here you can compensate the balance below the reservoir. If the balance (load cell) is slightly inaccurate, you can compensate for this with this setting. Default is 0.00%.
3. **Set reservoir defs:** Sets the weight trim back to 0.00%. The tare is not changed.

## Boiler PID

These are the settings of the temperature controller (PID controller) of the boiler. The controller sets the heating power from three parts: the proportional part (P), the integral part (I) and the derivative part (D). We have pretuned your machine, so normally you should not have to change these settings. If needed, you can always go back with Set boiler PID defs at the end of this submenu, or System > Reset defaults.

1. **P-Gain:** Proportional gain of the PID temperature controller. The proportional part gives more power the further the temperature is below the setpoint. A higher value reacts stronger, but can make the temperature swing. Default is 6.0.
2. **I-Gain:** Integral gain. The integral part slowly builds up as long as the temperature is not at the setpoint, so the setpoint is reached exactly. A higher value builds up faster, but can cause overshoot. Default is 0.05.
3. **D-Gain:** Derivative gain. The derivative part reacts to how fast the temperature changes and dampens the swing. Default is 150.
4. **DFF factor:** The Dynamic Feed Forward (DFF) Factor used for tuning the aggressiveness of the DFF. The DFF adds heating power during a shot based on the reservoir outflow, to compensate for the cold water that enters the boiler. Default is 70%, range: 0%-150%.
5. **DFF decay:** How fast the extra power of the DFF fades out after the shot. A lower value fades out faster. Default is 0.970, range: 0.900-1.000.
6. **Feedforward:** This is the static feed forward setting for the PID controller. This is the amount of power that is needed to float the temperature. This setting compensates for heat loss towards the environment. Default is 5.5%.
7. **Windup min:** The low limit of the integral part of the controller. This prevents the integral part from building up too far (wind-up). Default is -15%.
8. **Windup max:** The high limit of the integral part of the controller, see setting 7. Default is 8%.
9. **Integr. band:** The integral part of the controller only builds up when the temperature is within this band around the setpoint. Far from the setpoint, for example while heating up, the proportional part does the work. Only used when Cond. integr is ON. 0 = no band. Default is 4.0°C.
10. **Cond. integr:** Conditional integration. When ON, the integral part does not build up when the heater is already at full or zero power, or when the temperature is outside the Integr. band. This prevents overshoot. Default is ON.
11. **I-down fact.:** Above the setpoint, the integral part goes down this many times faster than it builds up. A higher value reduces overshoot. Default is 1.0 (same speed), range: 1.0-5.0.
12. **Brew I-hold:** When ON, the integral part is held during the temperature dip after a shot. This prevents overshoot after each shot. Default is ON.
13. **Hold timeout:** The Brew I-hold ends at the latest this long after the shot. Default is 180 s.
14. **Dip window:** The Brew I-hold only starts when the temperature drops below the setpoint within this time after the shot. Default is 60 s.
15. **Set boiler PID defs:** Sets settings 1-14 back to their defaults.

## Steam PID [diyPresso Two only]

These are the settings of the temperature controller of the thermoblock of the steamer. Like the boiler, we have pretuned your machine, so normally you should not have to change these settings. The steamer uses two sets of gains: one while the steam pump is off (heating up and ready) and one while the steam pump runs (steaming).

1. **P-Gain:** Proportional gain while the steam pump is off. See Boiler PID, setting 1. Default is 3.5.
2. **I-Gain:** Integral gain while the steam pump is off. Default is 0.05.
3. **D-Gain:** Derivative gain while the steam pump is off. Default is 50.
4. **Feedforward:** The feed forward for the thermoblock. Default is 3.0%.
5. **Pump P-Gain:** Proportional gain while the steam pump runs. Default is 5.0.
6. **Pump I-Gain:** Integral gain while the steam pump runs. Default is 0.10.
7. **Pump D-Gain:** Derivative gain while the steam pump runs. Default is 50.
8. **Windup min:** The low limit of the integral part, see Boiler PID, setting 7. Default is -5%.
9. **Windup max:** The high limit of the integral part. Default is 100%.
10. **Integr. band:** The band around the setpoint in which the integral part builds up, while the steam pump is off. See Boiler PID, setting 9. Default is 5.0°C.
11. **Pump I band:** The same, while the steam pump runs. 0 = no band. Default is 0.0°C.
12. **Cond. integr:** Conditional integration, see Boiler PID, setting 10. Default is ON.
13. **I-down fact.:** See Boiler PID, setting 11. Default is 3.0.
14. **Set steam PID defs:** Sets settings 1-13 back to their defaults.

## System

1. **Status:** Shows the machine status: the status of the brew process and of the boiler, and possible errors of the brew process, the boiler and the reservoir. Push the button to go back.
2. **WiFi:** To put the diyPresso in a WiFi network: OFF, ON or CONFIG-AP. (setting not yet activated)
3. **Knob dir:** The direction of the rotary push-turn button: NORMAL or REVERSE. With REVERSE, turning left and right are swapped. The new direction is used after saving.
4. **Commissioning:** Restarts the commissioning sequence. Push the button and confirm with YES. The menu closes and the commissioning starts.
5. **Reset defaults:** Back to factory settings. Push the button and confirm with YES. The tare and weight trim of the reservoir and the commissioning status are kept.
6. **Firmware:** Shows the firmware (software) version, for example 2.0.0-B2.
7. **Hardware:** Shows the machine model and hardware revision, for example "One r1" or "Two r1".

---

# Notes for the editor (not for the guide)

- The values in the screen examples (98.0°C, 1234 shots, 1/7) are examples. On the One, the main menu has 5 items (1/5): Brew, Reservoir, Boiler PID, System, < Exit.
- Default values are those of firmware 2.0.0. If you would rather not print defaults for the advanced (PID) settings, they can be left out.
- "Extraction weight" is removed from the menu. It can be removed from the guide.
- The WiFi text "(setting not yet activated)" is kept from the current guides. Please check if this is still correct for this release.
- The link https://pidexplained.com/how-to-tune-a-pid-controller/ in the current guides no longer works. It is removed and replaced by a short explanation of the proportional, integral and derivative part.
