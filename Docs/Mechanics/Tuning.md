# Player movement tuning

Where every player movement, camera, input and animation value lives, and what it does.
Test map: `Content/Developers/Nick/Lvl_NickMovementTest` (Play to try changes).

**How to tune**
- **Data assets** (`Content/Mechanics/Data/`): open, change, save. Values are read every frame, so most changes show up even while playing.
- **Anim Blueprint** (`Content/Mechanics/Animation/ABP_CoralisPlayer`): open, **Class Defaults** button in the toolbar, change, **Compile**, save.
- **Input keys** (`Content/Mechanics/Input/IMC_Coralis_KBM`, `IMC_Coralis_Gamepad`): open the mapping context, change the key next to an action, save.

---

## Tune list (things to revisit)

Keep this list up to date: add a line whenever something feels off, and note what was tried.

| What | Where | Setting(s) | Now | Notes |
|---|---|---|---|---|
| Mouse sensitivity | DA_PlayerInput | Mouse Degrees Per Unit | 1.5 | Was 2.5. Gamepad: Gamepad Yaw Rate / Pitch Rate. |
| Mouse invert (test key Y) | DA_PlayerInput | Invert Mouse Y / Invert Gamepad Y | off | Y flips mouse look while playing until the options menu exists. |
| Start-up weight (peek spam) | DA_PlayerMovement > Weight | Startup Curve, Acceleration | keys 0→0.32, 0.053→0.749, 0.622→0.958, 1→1 (Nick, 2026-11-22); 700 | Slow first step, then a quick rise to ~75% acceleration. Lower the left end for a heavier first step. |
| A/D reversal | DA_PlayerMovement > Weight | Reverse Deceleration | 900 | Lower = spamming A/D gets you nowhere. |
| Turn weight | DA_PlayerMovement > Weight | Turn Acceleration | 1000 | Lower = wider, heavier direction changes. |
| Stop slide | DA_PlayerMovement > Ground | Braking Deceleration, Braking Friction | 1100, 2 | Lower = longer slide when letting go. |
| Roll distance / time | DA_PlayerMovement > Dodge | Dodge Distance, Dodge Duration | 600 cm, 1 s | Placeholder dash animation is stretched to the duration. |
| Roadie run lean | ABP_CoralisPlayer Class Defaults > Coralis\|Sprint | Sprint Lean In Time, Sprint Lean Out Time, Sprint Lean Degrees Per Bone | 0.5 s, 0.3 s, 15° (Nick, 2026-11-22) | Applied to spine_01..03 (3 bones = 45° total). |
| Walk → sprint camera | DA_PlayerCamera > State Profiles > Sprinting | Blend Speed | 2.5 | Lower = slower camera change into the sprint view. |
| Sprint camera steering | DA_PlayerCamera > State Profiles > Sprinting | Heading Follow Speed | 5 | How strongly the camera swings with the run. 0 = free camera. |
| Mouse vs. sprint steering | DA_PlayerCamera | Heading Follow Look Pause, Heading Follow Resume Time | 0.5 s, 0.4 s | Follow pauses while you move the mouse, then fades back in. |
| Sprint look sensitivity | DA_PlayerCamera > State Profiles > Sprinting | Look Sensitivity Scale | 0.5 | Half-speed look while sprinting. |
| Jog clip switches (back ↔ side ↔ forward) | ABP_CoralisPlayer AnimGraph | Blend Time on the three "Blend Poses by bool" jog switches | 0.25 s | Cross-fade between the four jog clips. Longer = softer pivot. |
| Hips swinging the wrong way around backpedal switches | ABP_CoralisPlayer Class Defaults > Coralis\|Locomotion | Max Back Clip Warp Angle | 20° | Lower = less wrong-way swing, a bit more foot slide on back diagonals. |
| How far each jog clip turns its legs | ABP_CoralisPlayer Class Defaults > Coralis\|Locomotion | Max Clip Warp Angle | 50° | Forward and side clips. |
| Leg turn speed | ABP_CoralisPlayer Class Defaults > Coralis\|Locomotion | Warp Turn Rate | 360°/s | How fast the legs follow a direction change. |
| Standing: torso turn before the legs follow | DA_PlayerMovement > Ground | Idle Torso Turn Limit, Idle Catch Up Rate, Idle Catch Up Stop Angle | 70°, 400°/s, 5° | Feet stay planted until the camera is past the limit, then the legs turn to face it. A turn-in-place step animation would make the catch-up look better (not needed for it to work). |
| Torso twist | ABP_CoralisPlayer Class Defaults > Coralis\|Aim | Torso Twist Limit, Torso Twist Rate | 90°, 720°/s | How far and how fast the upper body twists toward the camera. |
| Sprint + back | DA_PlayerMovement > Sprint | Sprint Cancel Input Angle | 150° | Sprint can't start (and stops) with the stick this far from the facing, so sprint + back just walks back. |
| Held A / D while sprinting | DA_PlayerMovement > Sprint | Sprint Live Steer Angle | 100° | A and D keep turning the run for as long as they are held. How fast depends on Sprint Turn and Heading Follow Speed. |
| Aim / crouch speed | DA_PlayerMovement | Aim Speed, Crouch Speed | 150, 200 (Nick, 2026-11-22) | Was 250 / 220. |
| Weapon sounds | DA_PlasmaRifle | Fire Sound | RifleShot | Placeholder: replace the sounds later. |

---

## DA_PlayerMovement (`Content/Mechanics/Data/DA_PlayerMovement`)

**Ground**
| Setting | Now | What it does |
|---|---|---|
| Jog Speed | 300 | Full-stick speed. Keep on 300: the jog animations are made for it (foot sliding otherwise). |
| Min Analog Speed | 40 | Speed at the lightest stick push. |
| Acceleration | 700 | Pick-up at full pace (the Startup Curve scales it down at low speed). |
| Braking Deceleration | 1100 | Slow-down when letting go. |
| Ground Friction | 6 | Direction change while sprinting (and jogging if Weighted Movement is off). |
| Braking Friction | 2 | Extra slow-down when letting go. |
| Lock Facing To Camera | on | Locked view: the character faces where the camera looks (except while sprinting or rolling). |
| Lock Catch Up Angle / Rate | 30°, 1080°/s | After a sprint or roll, how the facing swings back to the camera. |
| Idle Torso Turn Limit | 70° | Standing: the feet stay planted until the camera turns past this; the torso twists meanwhile. |
| Idle Catch Up Rate / Stop Angle | 400°/s, 5° | Standing: how the legs turn to catch up with the camera, and when they stop. |
| Jog Turn | 600/120/45 | Facing turn toward the move direction when the view is not locked. |

**Weight**
| Setting | Now | What it does |
|---|---|---|
| Weighted Movement | on | Off = the old snappy movement (handy for comparing). |
| Startup Curve | 0→0.32, 0.053→0.749, 0.622→0.958, 1→1 (cubic) | X = speed as a fraction of full pace, Y = how much of Acceleration you get. |
| Reverse Deceleration | 900 | Braking when pushing against your current motion (A ↔ D). |
| Turn Acceleration | 1000 | How fast velocity bends sideways toward a new direction. |

**Aim / Crouch / Sprint**
| Setting | Now | What it does |
|---|---|---|
| Aim Speed | 150 | Move speed while aiming. |
| Aim Turn | 900/240/20 | Facing turn while aiming, when the view is not locked. |
| Crouch Speed | 200 | Move speed crouched. |
| Crouched Half Height | 60 | Capsule height crouched. |
| Sprint Speed | 600 | Roadie run speed. Keep on 600: the run animation is made for it. |
| Sprint Acceleration | 1000 | Pick-up into the sprint. |
| Sprint Turn | 140/60/30 | Facing turn while sprinting; low = wide, committed turns. |
| Sprint Cancel Input Angle | 150° | With the stick further than this from the facing the sprint stops or won't start (sprint + back walks back). |
| Sprint Live Steer Angle | 100° | Stick up to this far from forward steers from the live camera, so a held A or D keeps turning while the camera follows (needs Heading Follow Speed above 0). Further back, the run keeps the camera direction from when the stick was pulled back, turned only by look input. Keep below 135 or a held back diagonal circles. |

**Dodge (roll)**
| Setting | Now | What it does |
|---|---|---|
| Dodge Distance | 600 | Ground covered by one roll. |
| Dodge Duration | 1.0 s | Roll time. |
| Dodge Speed Curve | 1.3 → 1.1 → 0.35 | Speed shape over the roll (only the shape matters). |
| Dodge Turn Rate | 1440°/s | How fast the character faces the roll direction. |
| Recovery Duration | 0.3 s | After a roll: no sprint, no new roll, slowed. |
| Recovery Speed Scale | 0.35 | Speed multiplier during recovery. |
| Can Dodge From Crouch | on | Rolling from a crouch stands up. |
| Dodge Animation / Anim Slot / Blend Time | MM_Dash, FullBody, 0.08 | Roll animation (placeholder). |

**Air**: Air Control 0.2 (steering in the air).

---

## DA_PlayerCamera (`Content/Mechanics/Data/DA_PlayerCamera`)

**Default Profile** and each **State Profile** (Aiming, Sprinting, Crouching; the first matching state wins) have:
| Setting | Default / Aim / Sprint / Crouch | What it does |
|---|---|---|
| Arm Length | 170 / 120 / 200 / 160 | Distance behind the character (smaller = closer). |
| Socket Offset | (0,75,55) / (0,70,60) / (0,75,50) / (0,75,35) | Y = right of the character, Z = up. |
| Field Of View | 85 / 60 / 90 / 85 | Zoom. |
| Blend Speed | 8 / 14 / 2.5 / 8 | How fast the camera changes **into** this profile (lower = slower). |
| Position Lag Speed | 12 / 0 / 8 / 12 | Camera trailing the character (0 = locked on). |
| Bob Amplitude / Frequency | sprint only: (2.5, 3.5), 2.6 Hz | Sprint camera shake (side, up). |
| Look Sensitivity Scale | 1 / 0.55 / 0.5 / 1 | Look speed multiplier in this state. |
| Heading Follow Speed | 0 / 0 / 5 / 0 | Camera swings with the character's run direction. |

**Rig settings**
| Setting | Now | What it does |
|---|---|---|
| Heading Follow Look Pause / Resume Time | 0.5 s / 0.4 s | Follow backs off while you move the mouse/stick. |
| Shoulder Swap Speed | 10 | Shoulder swap blend. |
| Start On Right Shoulder | on | |
| Pitch Min / Max | -70° / 60° | Look down / up limits. |
| Probe Size | 12 | Camera collision radius. |
| No Lag Blend Speed | 40 | Internal: keeps the aim camera from popping. Leave as is. |

---

## DA_PlayerInput (`Content/Mechanics/Data/DA_PlayerInput`)

| Setting | Now | What it does |
|---|---|---|
| Mouse Degrees Per Unit | 1.5 | Mouse sensitivity. |
| Gamepad Yaw Rate / Pitch Rate | 220 / 140 °/s | Stick look speed at full tilt. |
| Gamepad Look Exponent | 1.6 | Stick response curve (higher = finer near center). |
| Invert Mouse Y / Invert Gamepad Y | off / off | Look inversion. |
| Evade Hold Threshold | 0.2 s | Space / A: tap = roll, hold longer than this = roadie run. |
| Evade Min Move Input | 0.3 | Minimum stick for a tap to roll (otherwise it rolls forward). |

**Keys** (`IMC_Coralis_KBM` / `IMC_Coralis_Gamepad`): WASD / left stick move, mouse / right stick look,
Space / A evade (tap roll, hold sprint), Left Shift sprint, RMB / LT aim, C / L3 crouch, Q / R3 shoulder swap,
Y invert mouse look (test). Dodge has no dedicated key for now.

---

## ABP_CoralisPlayer Class Defaults (`Content/Mechanics/Animation/ABP_CoralisPlayer`)

**Coralis|Locomotion**

Jogging (not aiming, not sprinting) plays four branches: forward, right, back, and left. Left plays the starter-pack
left jog as-is, with no leg turn (placeholder until a proper strafe-left clip is found; swap it on the left
Sequence Player in the AnimGraph). Each branch turns
its own legs toward the travel direction, and the branches cross-fade, so every blend is between two poses that
already line up. (`MDT_Mannequin` mirror table is left over from a mirrored-left attempt and not used.)

| Setting | Now | What it does |
|---|---|---|
| Max Clip Warp Angle | 50° | How far the forward and side clips turn their legs. |
| Max Back Clip Warp Angle | 20° | How far the backpedal (and the side clips toward the back) turn their legs. Small keeps the hips from swinging the wrong way. |
| Warp Turn Rate | 360°/s | How fast the legs follow a direction change. |
| Cardinal Clip Speed | 290 cm/s | Speed the jog clips are made for; their play rate follows your actual speed. |
| Min / Max Loco Play Rate | 0.35 / 1.5 | Play rate limits for the jog clips. |
| Left Clip Travel Angle / Max Left Clip Warp Angle | -90° / 0° | Left branch: plays its own clip with no leg turn. |
| Use Left / Right Cardinal | on / on | Whether the left/right branches are used. |
| Cardinal Hysteresis | 10° | Keeps a held diagonal on one clip. |
| Side / Back Cardinal Bias | 5° / 0° | Prefer forward/back over a side clip on exact diagonals. |
| Direction Blend Rate | 540°/s | Direction smoothing for the aiming walk blendspace. |
| Move Speed Threshold | 3 | Below this the character counts as standing. |

**Coralis|Aim**
| Setting | Now | What it does |
|---|---|---|
| Torso Twist Limit | 90° | Largest upper-body twist toward the camera. |
| Torso Twist Rate | 720°/s | How fast the twist follows (also smooths it on other players' screens). |

**Coralis|Sprint**
| Setting | Now | What it does |
|---|---|---|
| Sprint Lean In Time | 0.5 s | Lean forward when the roadie run starts. |
| Sprint Lean Out Time | 0.3 s | Straighten up when it ends. |
| Sprint Lean Degrees Per Bone | 15° | Forward bend on each of spine_01..03 (45° total). |

The anim Blueprint also has **Root Motion Mode = Ignore Root Motion** (rolls are driven by code, not the animation). Leave it.

---

# Rifle and heat tuning

Test it in `Lvl_NickMovementTest`: the three target dummies show their health and the last hit.
Console: `Coralis.Weapon.SetHeat 0.65` (0 to 1), `Coralis.Weapon.Fire`, `Coralis.Weapon.Vent`,
`Coralis.Weapon.ShowDebug 1` (heat and sweet spot window on screen).

**Heat rules**
- Each shot adds heat. Passive cooling starts after a short delay.
- Full heat jams the rifle for a fixed time. Vent does nothing during a jam, and the trigger must be pulled again afterwards.
- Vent can be tapped at any heat above zero. A vent always runs down to zero, and the rifle can't fire until it gets
  there. By default the trigger has to be pulled again after a vent.
- Pressing vent while the heat is inside the marked window is a sweet spot: faster cooling and a damage buff. Outside
  the window it is a normal vent. There is one attempt per vent. The buff starts at the press.
- The window lands at a random place between 60% and 96% heat. It moves after every vent and every jam. Cooling on its
  own doesn't move it. The marks are hidden while venting or jammed.
- Cooling must stay ordered, slowest first: jam < passive < vent < sweet spot (data validation on the asset checks
  this, and that the window fits its range).

## DA_PlasmaRifle (`Content/Mechanics/Data/DA_PlasmaRifle`)

| Setting | Now | What it does |
|---|---|---|
| Fire Rate | 9 shots/s | Rate while the trigger is held. |
| Damage | 12 | Damage of a body hit, before zone and buff multipliers. |
| Range | 10000 cm | Longest shot. |
| Trace Channel | GameTraceChannel1 | Collision channel the shots trace on. |
| Friendly Fire | off | Whether shots damage other players. |
| Muzzle Offset | (50, 25, 35) | Shot origin from the character: forward, right, up. Cover between it and the aim point blocks the shot. |
| Max Heat / Heat Per Shot | 100 / 4 | 25 shots from cold to a jam. |
| Passive Cool Delay / Rate | 1 s / 35 per s | Cooling when left alone. |
| Jam Duration | 4 s | Lockout at full heat (cools 100 in this time, so 25 per s). |
| New Trigger Pull After Jam | on | Off = a held trigger starts firing again when the jam ends. |
| New Trigger Pull After Vent | on | Off = a held trigger starts firing as soon as the vent reaches zero. |
| Vent Cool Rate | 55 per s | Normal vent. |
| Sweet Spot Range Start / End | 0.6 / 0.96 | The window lands at random inside this range (fraction of max heat). 0.96 is the highest heat that doesn't jam at 4 per shot. |
| Sweet Spot Width | 0.2 | Size of the window as a fraction of max heat. 0 = no sweet spot. |
| Sweet Spot Cool Rate | 110 per s | Cooling after a sweet spot. |
| Buff Damage Multiplier / Duration | 1.35 / 5 s | Sweet spot damage buff. |
| Hip Spread / Aim Spread | 2.5° / 0.5° | Half angle of the shot cone. |
| Recoil Pitch / Yaw | 0.3° / 0.15° | Camera kick per shot (yaw is random to either side). |
| Aim Recoil Scale | 0.7 | Recoil multiplier while aiming. |
| Fire Sound | RifleShot (`Content/SFX/GunsSFX/Rifle`) | Played on each shot. Placeholder, to be replaced. |
| Tracer Duration / Color | 0.05 s / (80, 220, 255) | Placeholder tracer. |
| Max Aim Origin Distance | 700 cm | Network check: how far from the character a client's shot may start. |
| Client Clock Tolerance | 0.25 s | Network check: allowed drift in the timing of a client's shots and vents. |
| Server Move State Grace | 0.25 s | How long the server waits for a client's movement state (for example sprint ended) before refusing a shot. |

## DA_HitZones_Humanoid (`Content/Mechanics/Data/DA_HitZones_Humanoid`)

| Zone | Damage multiplier | Bones (and everything below them) |
|---|---|---|
| Head | 2.0 | head, neck_01 |
| Arm | 0.7 | upperarm_l, upperarm_r |
| Leg | 0.7 | thigh_l, thigh_r |
| Body | 1.0 | everything else |

Zones come from the bodies of the character's physics asset.

## Health (Coralis Health component) and target dummy

| Setting | Where | Now |
|---|---|---|
| Max Health | Health component | 100 |
| Hit Zones | Health component | DA_HitZones_Humanoid on the dummy |
| Reset Delay | BP_TargetDummy (`Content/Developers/Nick`) | 2 s until its health refills |

## HUD and crosshair (BP_CoralisPlayer > Weapon HUD component)

The heat bar and label are the ones in `Content/UI/HUD/WBP_WeaponHUD`, fed from code. In that widget the label sits on
its own line above the bar, so the bar keeps its size whatever the text says. Bar height: Height Override on `SizeBox_0`
(27), the box around the bar.

| Setting | Now | What it does |
|---|---|---|
| HUD Widget Class, HUD Z Order | WBP_WeaponHUD, 0 | Which HUD is created for the local player. |
| Heat Bar Name / Heat Label Name | ProgressBar_Heat / Txt_HeatLabel | Names looked up inside that widget. |
| Ready / Venting / Sweet Spot / Jammed Text | HEAT / VENTING / SWEET SPOT / JAMMED | Label text per state. |
| Buff Suffix | "  +DMG {Seconds}s" | Added to the label while the buff runs. |
| Ready / Venting / Sweet Spot / Jammed Color | orange / blue / green / red | Bar color per state. |
| Crosshair Z Order | 10 | Draw order of the crosshair overlay. |

**Crosshair** (same component, `Crosshair` group)

| Setting | Now |
|---|---|
| Line Length, Thickness | 8, 2 |
| Hip Gap / Aim Gap | 14 / 5 |
| Color / Blocked Color (sprinting, rolling, venting, jammed) | white 85% / white 25% |
| Outline Color, Outline Width | black 70%, 1 |
| Hit Marker Duration, Inner, Outer | 0.18 s, 7, 15 |
| Hit / Head Hit / Limb Hit / Depleted Color | white / yellow / pale blue / red |
| Sweet Spot Mark Color, Thickness, Overhang | green, 2, 3 |

**Keys:** fire LMB / RT, aim RMB / LT, vent R / X (`IMC_Coralis_KBM` / `IMC_Coralis_Gamepad`).

**Not done yet:** shots start on the right side even when the camera is over the left shoulder.
