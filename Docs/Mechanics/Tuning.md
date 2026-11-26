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
