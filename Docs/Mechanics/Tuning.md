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
Space / A evade (tap roll, hold sprint), Left Shift sprint, RMB / LT aim, C / L3 crouch,
Q / R3 shoulder swap (also changes hands), Y invert mouse look (test). Dodge has no dedicated key for now.

## Weapon hand (shoulder swap)

Swapping the camera shoulder also moves the weapon to the other side: the whole armed pose, legs included, is mirrored
(table `Content/Mechanics/Animation/MDT_Mannequin`), and shots start on that side. Left handed, the animation plays the
clip for the mirrored travel direction, so after mirroring the legs still step the way the character moves. The aim bend
and sprint lean are added after the mirror, and rolls are not mirrored. Other players see it too. There is no rifle model
yet; when there is one, it has to change hand bones with this.

| Setting | Where | Now | What it does |
|---|---|---|---|
| Switch Hands With Shoulder | BP_CoralisPlayer Class Defaults > Coralis\|Stance | on | Off = only the camera changes side. |
| Blend Time (on the Mirror node) | ABP_CoralisPlayer AnimGraph | 0.2 s | Blend between the two stances. |

---

## ABP_CoralisPlayer Class Defaults (`Content/Mechanics/Animation/ABP_CoralisPlayer`)

**Coralis|Locomotion**

Jogging (not aiming, not sprinting) plays four branches: forward, right, back, and left. Left plays the starter-pack
left jog as-is, with no leg turn (placeholder until a proper strafe-left clip is found; swap it on the left
Sequence Player in the AnimGraph). Each branch turns
its own legs toward the travel direction, and the branches cross-fade, so every blend is between two poses that
already line up.

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
| Muzzle Offset | (50, 25, 35) | Shot origin from the character: forward, right, up (right becomes left when the weapon changes hands). Cover between it and the aim point blocks the shot. |
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

---

# Enemies, damage and death

Test map: `Content/Mechanics/Maps/Lvl_CoreLoop`, our copy of Panagiotis's greybox with our own wave director. Open it
and press Play. For two players: Play settings, Number of Players 2, Net Mode Play As Listen Server.

The horde mode (`Content/Mechanics/Core/BP_CoralisHordeBase`) is our copy of Daniel's Blueprint. Waves run from the
`WaveDirector_CoreLoop` actor in the map. Our copy of the spawner (`Content/Mechanics/AI/BP_CoralisEnemySpawner`) is
no longer placed. Daniel's originals are unchanged, so later changes to them don't carry over by themselves.

Console:
- `Coralis.Enemy.ShowDebug 1` (host): each enemy's state, health and fighting distance, where it is heading and its
  line to the target (red = it sees you).
- `Coralis.Enemy.KillAll`: kills every enemy that has arrived (host).
- `Coralis.Wave.Skip` (host): ends the current wave or countdown.
- `Coralis.Wave.Restart` (host): back to wave 1, with every player respawned.
- `Coralis.Health.Damage <amount>`, `Coralis.Health.Kill`: damage or kill your own character.

## Where to change what

| What | Asset |
|---|---|
| How enemies think, move, shoot and react | `Content/Mechanics/Data/DA_EnemyBehavior_Soldier` |
| **Run or rush**, dashing, grouping up | `DA_EnemyBehavior_Soldier`, groups **Rush**, **Dash**, **Group Up** |
| **Being shot at**, breaking contact, what to do in cover | `DA_EnemyBehavior_Soldier`, groups **Hit Reaction**, **Cover**, **In Cover** |
| **Enemy health** | `BP_CoralisEnemy` > Health component > Max Health (150) |
| How far and fast a dash goes (enemies) | `DA_EnemyMovement` > Dodge |
| Enemies arriving in groups | `DA_Waves_CoreLoop`, group **Groups** |
| Enemy rifle (damage, fire rate, heat, spread, tracer) | `Content/Mechanics/Data/DA_EnemyRifle` (same settings as DA_PlasmaRifle) |
| Enemy speeds and movement feel | `Content/Mechanics/Data/DA_EnemyMovement` (same settings as DA_PlayerMovement) |
| Enemy health, body, behaviour asset, corpse time | `Content/Mechanics/AI/BP_CoralisEnemy` |
| Enemy colors | `MI_Enemy_HeadLegs`, `MI_Enemy_Torso` in `Content/Mechanics/AI` |
| Player health, regeneration, respawn | `BP_CoralisPlayer` (Health component and Class Defaults) |
| Health bar | `BP_CoralisPlayer` > Health HUD component |
| Wave sizes, timing, spawn places, restart rule | `Content/Mechanics/Data/DA_Waves_CoreLoop` |
| Which wave asset runs, start on its own, centre of the spawn area | `WaveDirector_CoreLoop` actor in `Lvl_CoreLoop` |
| Wave panel | `BP_CoralisPlayer` > Wave HUD component |

For another kind of enemy, make a child of `BP_CoralisEnemy` and give it its own copies of the data assets.
Changes to the behaviour asset apply from the next decision, also while playing.

## How an enemy behaves
- **Target:** the nearest living player. A player who just hit it counts as closer.
- **Advance:** moves toward the target in uneven legs, jogging and able to shoot (**running**).
- **Rush:** every so often it may sprint at the target instead, without shooting. The chance depends on the distance.
- **Engage:** holds a fighting distance, strafes, shoots in bursts, vents when hot and now and then pushes closer.
- **Dash:** the same roll the players have. Forward while running or rushing, sideways while fighting, and along its
  way while heading to cover or fleeing. It only dashes where there is walkable ground for the whole roll.
- **Group Up:** an enemy on its own may join a friendly nearby. The two wait for each other, then go in together as a
  squad: same target, staying close, and rushing when the leader decides to.
- **Shot at:** a hit, or a shot passing close by, may make it **break contact** (**Move To Cover**): it walks to a spot
  the target can't see and shoots back on the way, from the hip with looser aim. With no such spot it backs off, still
  shooting. A hit that doesn't send it to cover may make it side roll. When badly hurt it may **Flee** instead: it
  sprints away without shooting.
- **In Cover:** it settles for a moment, then picks one: attack again, go and join friendlies, or wait there
  (5-10 s, then attack or join friendlies). It doesn't shoot from cover. Peeking comes with the cover system.
- A rushing enemy ignores near misses, and so does an enemy in cover that can't see its target.
- Cover is a placeholder until there is a cover system: any reachable spot hidden from the target.

## DA_EnemyBehavior_Soldier

Distances in cm.

| Group | Setting | Now | What it does |
|---|---|---|---|
| Targeting | Decision Interval | 0.2 s | Time between decisions. Aim updates every frame. |
| Targeting | Switch Distance Advantage | 300 | How much closer another player must be before the enemy turns to them. |
| Targeting | Attacker Priority Distance / Attacker Memory | 800 / 4 s | Whoever hit it counts as this much closer, for this long. |
| Targeting | Sight Channel | Visibility | What blocks sight, also used to judge cover. |
| Advancing | Preferred Range Min / Max | 700 / 1400 | Each approach picks a fighting distance in this range. |
| Advancing | Advance Leg Length / Advance Side Offset | 600 / 450 | Length of one step of the approach and how far it wanders sideways. |
| Advancing | Chase Range Factor | 1.25 | How far past the fighting distance the target may drift before the enemy follows. |
| Advancing | Lost Sight Delay | 0.8 s | Time without sight before it moves to get it back. |
| Advancing | Push Interval / Push Distance / Min Push Range | 4 s / 200 / 450 | While fighting, how often and how far it pushes closer, and the limit. Interval 0 = never. |
| Advancing | Acceptance Radius | 60 | How close counts as arrived. |
| Rush | **Rush Chance By Distance** | 0 → 0, 1200 → 0.15, 2200 → 0.5, 3500 → 0.85 | **Run or rush.** A list of distance bands (From Distance, Chance). The band the enemy is in gives the chance to start a rush at each decision. Add, remove or edit bands. Empty = never rushes. |
| Rush | Rush Decision Interval | 2 s | Time between two decisions. Shorter = rushes more often at the same chance. |
| Rush | Rush End Distance | 800 | A rush ends once the target is in sight and this close. None starts closer than this. |
| Rush | Rush Max Duration | 6 s | A rush that gets nowhere is dropped after this long. |
| Dash | Dash Decision Interval / Dash Cooldown | 1 s / 3 s | How often it considers a dash, and the least time between two. |
| Dash | Advance / Rush Dash Chance | 0.1 / 0.25 | Chance per decision of a forward dash while running closer / rushing. |
| Dash | Fight Dash Chance | 0.12 | Chance of a side dash while fighting. |
| Dash | Break Contact / Escape Dash Chance | 0.1 / 0.3 | Chance of a dash while breaking contact / fleeing. All dash chances 0 = only the dash from being hit (Hit Reaction > Dodge Chance). |
| Group Up | Can Group Up | on | Off = never goes looking for friendlies. |
| Group Up | Group Up Decision Interval / **Group Up Chance** | 3 s / 0.4 | How often an enemy on its own looks, and the chance it joins when there is an opportunity. |
| Group Up | Group Up Search Radius / Max Path Factor | 2000 / 1.5 | **Opportunity:** a friendly this near, with a walk of at most radius x factor... |
| Group Up | Group Up Max Backtrack | 600 | ...who is at most this much further from the player than the one joining. Fleeing and idle friendlies and full squads don't count. |
| Group Up | Squad Max Size | 4 | |
| Group Up | Gather Distance / Gather Timeout | 450 / 6 s | Gathered when everyone coming is this close to the leader; stops waiting after the timeout. |
| Group Up | Sprint To Group Up | on | |
| Group Up | Rush After Gather Chance | 0.6 | Chance the squad rushes as soon as it has gathered. |
| Group Up | Squad Cohesion Distance | 800 | A member further than this from the leader moves back to them while closing in. |
| Group Up | Squad Shares Target | on | The squad goes for the leader's target. |
| Strafing | Strafe Interval Min / Max | 0.8 / 2 s | Time between sideways moves. |
| Strafing | Strafe Distance Min / Max | 200 / 500 | Length of a sideways move. |
| Shooting | Can Shoot | on | Off = enemies only move. |
| Shooting | Fire Range | 6000 | Longest distance it shoots from. |
| Shooting | Reaction Time | 0.6 s | Target in sight this long before the first burst. |
| Shooting | Burst Duration Min / Max | 0.4 / 1.1 s | Length of a burst. |
| Shooting | Burst Pause Min / Max | 0.7 / 1.6 s | Pause between bursts. |
| Shooting | Aim When Shooting | off | Aiming stance while standing: tighter spread, slower. |
| Shooting | Aim Height Offset | 20 | Aim point above the middle of the target. |
| Shooting | Aim Error Degrees / Aim Error Interval | 3° / 0.35 s | **Main accuracy setting.** Higher error = more misses. |
| Shooting | Aim Turn Rate | 260°/s | How fast it swings its aim. |
| Shooting | Fire Angle Tolerance | 15° | Only fires once aimed within this angle. |
| Shooting | Vent At Heat | 0.7 | Vents between bursts past this heat. Above 1 = never. |
| Hit Reaction | Reaction Cooldown | 2 s | After a reaction, further hits and near misses are ignored for this long. |
| Hit Reaction | **Take Cover Chance** | 0.9 | Chance a hit makes it break contact. |
| Hit Reaction | **Incoming Fire Radius** | 200 | A player's shot passing this close counts as being shot at. 0 = only hits count. |
| Hit Reaction | **Near Miss Cover Chance** / Near Miss Cooldown | 0.6 / 1 s | Chance a near miss makes it break contact, and the least time between two such chances. |
| Hit Reaction | Near Miss Stops Rush | off | On = near misses can also turn back a rushing enemy. |
| Hit Reaction | Dodge Chance | 0.35 | If a hit doesn't send it to cover: chance of a side dash. |
| Hit Reaction | Flee Health Fraction / Flee Chance | 0.35 / 0.6 | Below this health it may run away, with this chance. |
| Cover | Shoot While Breaking Contact | on | Shoots back from the hip on the way. Needs sight of the target; never while sprinting or dashing. |
| Cover | Return Fire Delay | 0.2 s | How soon the first burst may start after breaking contact. |
| Cover | Break Contact Aim Error Degrees | 5° | Aim error while breaking contact (3° otherwise). |
| Cover | Cover Search Radius / Cover Samples | 1500 / 64 | How far it looks and how many spots it checks. With none found it backs off by Flee Distance. |
| Cover | Cover Hide Height | 100 | A spot counts when the target can't see this height above its floor. |
| Cover | Cover Min Target Distance | 450 | Ignores cover closer to the target than this. |
| Cover | Cover Max Path Factor | 1.6 | Longest walk to cover, times the search radius. |
| Cover | Crouch In Cover / Sprint To Cover | on / off | Sprint on = gets there faster but can't shoot on the way. |
| Cover | Cover Travel Timeout | 10 s | Gives up on reaching cover. |
| In Cover | Cover Settle Min / Max | 0.5 / 1.2 s | Pause before the first choice. |
| In Cover | **Cover Attack / Seek Group / Wait Weight** | 0.4 / 0.3 / 0.3 | How likely each choice is, compared with the others. With no friendly to join it attacks. |
| In Cover | **Cover Wait Min / Max** | 5 / 10 s | Length of a wait. |
| In Cover | Max Cover Waits | 1 | Waits allowed in one stay in cover. 0 = never waits. |
| Flee | Flee Distance / Flee Spread Angle | 1200 / 70° | How far it runs and how far off straight away it may go. |
| Flee | Flee Max Duration | 3.5 s | Longest it keeps running. |
| Flee | Sprint When Fleeing | on | |

## Enemy body, rifle and speeds

| Setting | Where | Now |
|---|---|---|
| Max Health | BP_CoralisEnemy > Health | 150 |
| Hit Zones | BP_CoralisEnemy > Health | DA_HitZones_Humanoid |
| Behavior | BP_CoralisEnemy Class Defaults > Coralis\|AI | DA_EnemyBehavior_Soldier |
| Corpse Life Span | BP_CoralisEnemy Class Defaults > Coralis\|Death | 5 s |
| Fire Rate / Damage / Heat Per Shot | DA_EnemyRifle | 5 per s / 6 / 6 |
| Jog / Aim / Crouch / Sprint Speed | DA_EnemyMovement | 260 / 200 / 200 / 470 |
| Sprint Cancel Input Angle | DA_EnemyMovement | 180° (enemies may sprint in any direction) |
| Dodge Distance / Duration (their dash) | DA_EnemyMovement > Dodge | 600 cm / 1 s |

## Player health and death

| Setting | Where | Now | What it does |
|---|---|---|---|
| Max Health | BP_CoralisPlayer > Health | 100 | |
| Hit Zones | BP_CoralisPlayer > Health | DA_HitZones_Humanoid | Same zones as on enemies. |
| Regen Rate / Regen Delay | BP_CoralisPlayer > Health > Regeneration | 20 per s / 5 s | Health comes back this long after the last hit. Rate 0 = off. |
| Invulnerable | BP_CoralisPlayer > Health | off | For testing. |
| Ragdoll On Death / Ragdoll Profile Name | Class Defaults > Coralis\|Death | on / Ragdoll | |
| Corpse Life Span | Class Defaults > Coralis\|Death | 0 | 0 = the body stays. |
| Respawn Delay | Class Defaults > Coralis\|Death | 6 s | **Placeholder.** 0 = stays dead. The wave director cancels it while Respawn Dead At Wave End is on, and during a restart. |

A dead character can't move, fire or vent, and enemies stop targeting it.

## Health bar (BP_CoralisPlayer > Health HUD component)

A placeholder at the bottom left, separate from the weapon HUD.

| Setting | Now |
|---|---|
| Show Health Bar, Z Order | on, 5 |
| Style > Margin, Size, Border | (40, 40), 320 x 22, 2 |
| Style > Back / Fill / Low Color | black 65% / green / red |
| Style > Low Fraction | 0.35 |
| Style > Flash Color, Flash Duration | white, 0.15 s |
| Style > Screen Flash Color, Screen Flash Duration | dark red 18%, 0.35 s |
| Style > Font, Text Color | Bold 12, white |
| Style > Dead Text | YOU DIED |

---

# Waves

A placeholder for the game's wave flow, run by the **wave director** actor in the map. The host decides everything.
A countdown leads into wave 1. A wave's enemies arrive at random moments over the round, alone or as a group, at random
places away from the players. When all of them are dead, a countdown leads to the next wave. When the run is lost, it
starts again from wave 1 with every player respawned.

## DA_Waves_CoreLoop (`Content/Mechanics/Data/DA_Waves_CoreLoop`)

Edits apply from the next wave.

| Group | Setting | Now | What it does |
|---|---|---|---|
| Enemies | Enemy Class | BP_CoralisEnemy | What spawns. |
| Wave Size | First Wave Enemies | 4 | |
| Wave Size | First Wave Increase / Increase Growth | 2 / 1 | Wave 2 adds this many, and each later step adds that much more: 4, 6, 9, 13, 18. Growth 0 = same step every wave. |
| Wave Size | Max Enemies Per Wave | 30 | |
| Wave Size | Fixed Wave Counts | empty | If filled, used for the first waves in order; the formula takes over after them. |
| Timing | First Wave Delay | 5 s | Countdown before wave 1, also after a restart. |
| Timing | Time Between Waves | 15 s | Countdown after the last enemy dies. |
| Timing | Round Duration | 60 s | **The game round:** a wave's arrivals are spread over this time, one at a random moment in each equal slice. |
| Timing | Max First Spawn Delay | 3 s | The first enemy arrives within this time. |
| Timing | Min Time Between Spawns | 1.5 s | |
| Timing | Max Empty Time | 8 s | With nobody left to fight and more to come, the next arrives within this time. 0 = keep the schedule. |
| Spawn Location | **Min Distance From Players** | 2000 cm | No enemy appears closer than this to a living player. |
| Spawn Location | Max Distance From Players | 0 | Furthest from the nearest living player. 0 = no limit. |
| Spawn Location | Spawn Radius | 6000 cm | Area around the director actor. Only places that can be walked to from it are used. |
| Spawn Location | Prefer Out Of Sight | on | Avoids places a player can see, if another one turns up. |
| Spawn Location | Min Distance From Other Enemies | 300 cm | |
| Spawn Location | Spawn Attempts / Spawn Retry Delay | 48 / 1 s | Places tried per enemy, and the wait if none fits. The distance rules are never relaxed. |
| Groups | **Group Chance** | 0.35 | Chance that an arrival is a group. 0 = always alone, 1 = always groups. |
| Groups | **Group Size Min / Max** | 2 / 4 | Enemies in a group. The last group of a wave may be smaller. |
| Groups | Group Spawn Radius / Group Member Spacing | 600 / 150 | How tightly a group appears. The distance-from-players rule holds for every member. |
| Groups | Groups Fight As Squad | on | The group shares a target, moves together and rushes together. Squad Max Size doesn't cap a spawned group. |
| Death | Restart Rule | Whole Squad Down | When the run goes back to wave 1. Whole Squad Down (alone: when you die), Any Player Dies, or Never. |
| Death | Restart Delay | 5 s | Time from the losing death to the restart. |
| Death | Respawn Dead At Wave End | on | A dead player waits until the wave is cleared. Off = the player's own Respawn Delay. |

**Wave director actor:** Settings (which wave asset), Start Automatically (on). The spawn area is centred on the actor,
so keep it on walkable ground. The map needs a navigation mesh over the whole area.

## Wave panel (BP_CoralisPlayer > Wave HUD component)

A placeholder at the top left: wave number, enemies remaining (including those still to arrive), the countdown to the
next wave or restart, and a note for a player waiting to return.

| Setting | Now |
|---|---|
| Show Wave Panel, Z Order | on, 5 |
| Style > Margin, Padding, Width | (40, 40), 12, 340 |
| Style > Back / Title / Text Color | black 55% / white / light grey |
| Style > Title Font, Font | Bold 18, Regular 13 |
| Style > Title / Remaining Text | WAVE {0} / Enemies remaining: {0} |
| Style > First Wave / Next Wave Text | First wave in {0} / Wave cleared. Next wave in {0} |
| Style > Restart / Waiting Text | Run lost. Restarting in {0} / You return when the wave is cleared |
