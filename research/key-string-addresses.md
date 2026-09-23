# Direcciones de strings y símbolos clave

Build principal: **BioRevHD 30-Enero-2013.exe** salvo indicación contraria.

Estas direcciones son evidencia de navegación/reconocimiento, no prueba por sí solas de semántica de gameplay.

## Enero 2013

| String | VA |
|---|---:|
| `UseOtomo` | `0x04CC2BF8` |
| `ForceTwoPlatoon` | `0x04CC2EE4` |
| `PickingCoopLock` | `0x04CC2FA0` |
| `NotSendPad` | `0x04CC2FE4` |
| `CreateRaidPlayer 5` | `0x04CC30E4` |
| `CreateRaidPlayer 4` | `0x04CC30FC` |
| `CreateRaidPlayer 3` | `0x04CC3114` |
| `CreateRaidPlayer 2` | `0x04CC312C` |
| `CreateRaidPlayer 1` | `0x04CC3144` |
| `mPartnerID` | `0x04CC3458` |
| `mSelfID` | `0x04CC3468` |
| `X(2P)` | `0x04CC62CD` |
| `Pad2` | `0x04CDC6DC` |
| `mPadNo` | `0x04CDC824` |
| `mCameraIdx` | `0x04CF3154` |
| `uCameraManage::setCameraIdx() : uPlayer::switchCamera()` | `0x04CF3250` |
| `for2P` | `0x04DB5308` |
| `Player 2` | `0x04DC4BE4` |
| `sGamePad` | `0x04E11D87` |
| `mIsUsePadEx` | `0x04E11DC0` |
| `mStartPadNo` | `0x04E11DD0` |
| `sGamePad::PadData` | `0x04E12210` |

También se encontraron, sin registrar aquí una VA individual para cada aparición:

- `uPcsActor<Player2>`
- `uPcsPlayerMain`
- `uPcsPlayerSub0`
- `uPcsPlayerSub1`
- `Main Player`
- `Sub Player 0`
- `Sub Player 1`
- `mMovePcs`
- `mMoveSubPcs`
- `VIEW_0..VIEW_7`
- `REGION_FULLSCREEN`
- `REGION_TOP`
- `REGION_BOTTOM`
- `REGION_LEFT`
- `REGION_RIGHT`
- regiones de cuadrante
- `PS2Pad2`
- `mPadViewportNo`
- `Self View`
- `Partner View`

## Febrero 2013

| String | VA |
|---|---:|
| `UseOtomo` | `0x011ACA9C` |
| `ForceTwoPlatoon` | `0x011ACD40` |
| `PickingCoopLock` | `0x011ACDE8` |
| `NotSendPad` | `0x011ACE20` |
| `mPartnerID` | `0x011ACF90` |
| `mSelfID` | `0x011ACF9C` |
| `CreateRaidPlayer 5` | `0x011AD0B0` |
| `CreateRaidPlayer 4` | `0x011AD0C4` |
| `CreateRaidPlayer 3` | `0x011AD0D8` |
| `CreateRaidPlayer 2` | `0x011AD0EC` |
| `CreateRaidPlayer 1` | `0x011AD100` |
| `mPadNo` | `0x011B2A74` |

La diferencia decisiva no fue la presencia del string, sino el código alcanzado: la ruta de febrero conserva wrapper/menu pero el destino funcional investigado termina en un stub `0x0079C2D0: ret 4`.

## sPcsManager — direcciones funcionales aproximadas

```text
mDebugPlayerMainID getter   0x02DCDF10
mDebugPlayerSub0ID getter   0x02DCDF50
mDebugPlayerSub1ID getter   0x02DCDF90

mIsDebug setter             ~0x02DD1460
MainID setter               ~0x02DD14C0
Sub0ID setter               ~0x02DD1520
Sub1ID setter               ~0x02DD1580

thunk común                 0x01BC1F58
implementación común        0x02DD16F0

mMovePcs xrefs              ~0x02DCB123, ~0x02DCB2A1
mMoveSubPcs xrefs           ~0x02DCB185, ~0x02DCB2ED
callbacks/getters           ~0x02DCB370, ~0x02DCB820,
                            ~0x02DCB3B0, ~0x02DCB8C0
```

## uCameraManage / sGamePad

```text
uCameraManage constructor   ~0x0206A070
uCameraManage::mPadNo       +0x88
uCameraManage activation    ~0x02069AB0

sGamePad constructor        ~0x02DAC0F0
sGamePad::mStartPadNo       +0x970
input route reading it      ~0x02DAE020
candidate data regions      +0x668, +0x7E8
```
