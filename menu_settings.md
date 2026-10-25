# diyPresso One & Two: menu structure and settings

Text for the Quick Installation & User Guide, firmware 2.0.0 (currently 2.0.0-B2).

This text replaces the sections "Menu structure and instructions" (last paragraph) and "Other settings" of:

- diyPresso One quick installation & user guide V1.1.3
- diyPresso Two quick installation & user guide V1.1.2

It is written for both guides. Parts marked **[Two only]** are only for the diyPresso Two guide: leave them out of the One guide. The menu of the One simply does not show these items.

## What changes compared to the current guides

- The menu is no longer one long list of 20 (One) or 25 (Two) pages. It now has a main menu with submenus, and shows 3 items at a time.
- There is no `<SAVE>` page anymore. When you leave the menu after changing something, the machine asks "Save changes?".
- Removed from the menu: FF-heat Value (the feed forward is now one setting, "Feedforward"), Extraction weight, Zero Counter (now done from the Shot counter), Commissioning status (now "Commissioning" in System), `<EXIT>` and `<SAVE>`.
- New in the menu: Knob direction, Status, Firmware, Hardware, and the advanced settings in Boiler PID and Steam PID.
- Tare, resetting the shot counter, commissioning and reset to defaults now ask "Are you sure?" first.
- Main screen: turning the button no longer opens the 'machine status' page. This page is now in the menu: System > Status.
  Remove the sentence "Turning the (right) button in the main screen will bring you to the 'machine status' page."
- Main screen: weight and water level indicator are one position further to the right. The screen pictures can stay as they are.
- The start-up screen now only shows the diyPresso logo (no version, date and time).
- The screen pictures of the settings menu ("SETTINGS [ 1/25] Temperature" and "<SAVE>") must be replaced. The screens below can be used for new pictures. On the display, `->` is a small arrow and `°` is the degree sign.

---

# Text for the guide

## Settings menu

Push the button right of the screen to enter the menu (One: "Push the button next to the screen"). The main menu shows the submenus. On the top right, you can see your position in the menu.

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

The menu has these submenus:

| Submenu    | Contents                                                      |
|------------|---------------------------------------------------------------|
| Brew       | brew settings and shot counter                                |
| Steam      | steam settings **[Two only]**                                 |
| Reservoir  | tare and weight trim of the reservoir                         |
| Boiler PID | temperature controller of the boiler (advanced)               |
| Steam PID  | temperature controller of the steamer (advanced) **[Two only]** |
| System     | status, WiFi, knob direction, commissioning, reset and version |

## Brew

1. **Temperature:** This is the value for the temperature setpoint of the boiler. Default is 98.0°C.
2. **Pre-infusion:** This is the pre-infusion time, the period that the pump is on to wet the puck. Default is 3.0 s.
3. **Infusion:** Infusion time indicates the time the pump is off and the puck can settle. Default is 1.0 s.
4. **Extraction:** Extraction time or shot time is the time the pump is on again after pre-infusion and infusion, actually extracting the espresso. Default is 25.0 s.
5. **Shot counter:** Counts the shots that have been made. Push the button to reset the counter to 0. The screen shows the current count and asks "Reset counter?".

   ```
   Shot counter    1234
   Reset counter?

      [NO]   YES
   ```

## Steam [Two only]

1. **Temperature:** This is the value for the temperature setpoint of the thermoblock. Default is 165.0°C.
2. **Pump power:** Adjust the power for the steam pump. Standard is 100%.

## Reservoir

1. **Tare (full):** Here you can tare the balance of the reservoir. Fill up the reservoir, looking on the side of the diyPresso so you can make sure not to overflow the reservoir. When the reservoir is full, push the button on this item and confirm with YES. The diyPresso will know the reservoir is tared and will after this stop automatically when the water runs out.
2. **Weight trim:** Here you can compensate the balance below the reservoir. If the balance (load cell) is slightly inaccurate, you can compensate for this with this setting. Default is 0.00%.

## Boiler PID

These are the settings of the temperature controller (PID controller) of the boiler. See https://pidexplained.com/how-to-tune-a-pid-controller/ for an explanation and tuning instructions. We have pretuned your machine, so normally you should not have to change these settings. If needed, you can always go back with System > Reset defaults.

1. **P-Gain:** P-Gain setting of the PID temperature controller. Default is 6.0.
2. **I-Gain:** I-Gain, see setting 1. Default is 0.05.
3. **D-Gain:** D-Gain, see setting 1. Default is 150.
4. **DFF factor:** The Dynamic Feed Forward Factor used for tuning the aggressiveness of the DFF. The DFF adds heating power during a shot, to compensate for the cold water that enters the boiler. Default is 70%, range: 0%-150%.
5. **DFF decay:** How fast the extra power of the DFF fades out after the shot. A lower value fades out faster. Default is 0.970, range: 0.900-1.000.
6. **Feedforward:** This is the feed forward setting for the PID controller. This is the amount of power that is needed to float the temperature. Because the diyPresso is hotter than its environment, a small portion of heat is always lost to the surroundings. This setting should compensate for this loss. Using a feed forward value instead of relying merely on the PID settings helps lowering the temperature swing of the boiler. It is used while heating up and when the temperature is reached. Default is 5.5%.
7. **Windup min:** The lowest value of the I-part of the controller. This prevents the I-part from building up too far (wind-up). Default is -15%.
8. **Windup max:** The highest value of the I-part of the controller, see setting 7. Default is 8%.
9. **Integr. band:** The I-part of the controller only builds up when the temperature is within this band around the setpoint. Far from the setpoint, for example while heating up, the P-part does the work. Only used when Cond. integr is ON. 0 = no band. Default is 4.0°C.
10. **Cond. integr:** Conditional integration. When ON, the I-part does not build up when the heater is already at full or zero power, or when the temperature is outside the Integr. band. This prevents overshoot. Default is ON.
11. **I-down fact.:** Above the setpoint, the I-part goes down this many times faster than it builds up. A higher value reduces overshoot. Default is 1.0 (same speed), range: 1.0-5.0.
12. **Brew I-hold:** When ON, the I-part is held during the temperature dip after a shot. This prevents overshoot after each shot. Default is ON.
13. **Hold timeout:** The Brew I-hold ends at the latest this long after the shot. Default is 180 s.
14. **Dip window:** The Brew I-hold only starts when the temperature drops below the setpoint within this time after the shot. Default is 60 s.

## Steam PID [Two only]

These are the settings of the temperature controller of the thermoblock of the steamer. Like the boiler, we have pretuned your machine, so normally you should not have to change these settings. The steamer uses two sets of gains: one while the steam pump is off (heating up and ready) and one while the steam pump runs (steaming).

1. **P-Gain:** P-Gain while the steam pump is off. See Boiler PID, setting 1. Default is 3.5.
2. **I-Gain:** I-Gain while the steam pump is off. Default is 0.05.
3. **D-Gain:** D-Gain while the steam pump is off. Default is 50.
4. **Feedforward:** The feed forward for the thermoblock, see Boiler PID, setting 6. Default is 3.0%.
5. **Pump P-Gain:** P-Gain while the steam pump runs. Default is 5.0.
6. **Pump I-Gain:** I-Gain while the steam pump runs. Default is 0.10.
7. **Pump D-Gain:** D-Gain while the steam pump runs. Default is 50.
8. **Windup min:** The lowest value of the I-part, see Boiler PID, setting 7. Default is -5%.
9. **Windup max:** The highest value of the I-part. Default is 100%.
10. **Integr. band:** The band around the setpoint in which the I-part builds up, while the steam pump is off. See Boiler PID, setting 9. Default is 5.0°C.
11. **Pump band:** The same, while the steam pump runs. 0 = no band. Default is 0.0°C.
12. **Cond. integr:** Conditional integration, see Boiler PID, setting 10. Default is ON.
13. **I-down fact.:** See Boiler PID, setting 11. Default is 3.0.

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
