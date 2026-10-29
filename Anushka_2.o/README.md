# Anushka_2.o

Anooshka is a humanoid robot codebase with modules for hearing, speech, hands, base movement, eye control, jaw sync, neck motion, vision, and monitoring.

## 2026 runtime refresh

The active runtime under `Sean/` has been modernized to remove the old hard dependency on `D:/Sean/...` and Windows `.bat` launchers.

Key changes:

- repo-relative paths instead of hardcoded drive letters
- environment-driven COM port configuration
- safer file-based IPC using newline-delimited control/status messages
- OpenAI Python SDK migration from legacy `Completion.create(...)` to the modern client
- multilingual speech input via OpenAI transcription when `OPENAI_API_KEY` is configured
- same-language replies by default, with explicit language requests honored
- graceful simulation mode when hardware ports are unavailable
- 3-Mega hardware layout support:
  left arm Mega, right arm Mega, and one shared aux Mega for jaw, eye, neck, and wheel base

## Setup

1. Create an environment file from `.env.example`.
2. Install dependencies from `requirements.txt`.
3. Set serial ports and API keys in `.env`.
4. Flash `Sean/Arduino-Progs/megaArmController/megaArmController.ino` to each arm Mega (`ARM_IS_LEFT=1` for the left board, `0` for the right). Flash `Sean/Arduino-Progs/megaAuxController.ino` only to a compatible Mega board.
5. Start the robot runtime with:

```bash
python Sean/BOSS/system_orchestrator.py
```

## Important environment variables

```env
OPENAI_API_KEY=
OPENAI_CHAT_MODEL=gpt-4o-mini
OPENAI_TRANSCRIBE_MODEL=gpt-4o-mini-transcribe
OPENAI_TRANSCRIBE_PROMPT=The speaker is speaking English in India. Robot name: Anooshka. Institution: KIET Group of Institutions in Ghaziabad.
OPENAI_REQUEST_TIMEOUT=20
OPENAI_MAX_RETRIES=1

ANUSHKA_TTS_ENGINE=pyttsx3
ANUSHKA_TTS_MODEL=gpt-4o-mini-tts
ANUSHKA_TTS_VOICE=nova
ANUSHKA_TTS_SPEED=0.95
ANUSHKA_TTS_RATE=145

ANUSHKA_MIC_DEVICE_INDEX=
ANUSHKA_MIC_NAME_HINT=ReSpeaker
OPENAI_TRANSCRIBE_LANGUAGE=en
ANUSHKA_STT_TIMEOUT=8
ANUSHKA_CAMERA_INDEX=0
ANUSHKA_CAMERA_NAME_HINT=Kreo
ANUSHKA_ENABLE_MONITORING=true
ANUSHKA_ENABLE_VISION=false

ANUSHKA_LEFT_ARM_MEGA_PORT=/dev/serial/by-id/<left-Mega-device-link>
ANUSHKA_RIGHT_ARM_MEGA_PORT=/dev/serial/by-id/<right-Mega-device-link>
ANUSHKA_AUX_MEGA_PORT=/dev/serial/by-id/<aux-Mega-device-link>
```

For your hardware:

- `ReSpeaker Quad Mic Array USB` can usually be selected automatically via `ANUSHKA_MIC_NAME_HINT=ReSpeaker`; matching devices are checked before selection
- Set `OPENAI_TRANSCRIBE_LANGUAGE=en` to keep English speech in English instead of automatic language detection; for English, the hearing path also skips a redundant translation request. Use `auto` or leave it empty for multilingual detection.
- `ANUSHKA_STT_TIMEOUT` bounds the Google speech-recognition fallback request; `OPENAI_REQUEST_TIMEOUT` and `OPENAI_MAX_RETRIES` bound/retry OpenAI requests. The transcription prompt can include the robot and institution names to help recognition.
- Use each board's `/dev/serial/by-id/` link rather than `/dev/ttyACM0`, `/dev/ttyACM1`, etc.; the ACM numbers can change after reconnect/reboot. Run `ls -l /dev/serial/by-id/` to identify persistent links. The arm controller retries opening and reconnects to these configured paths if a Mega is temporarily absent.
- The arm sketch expects Arduino Mega boards; its arm/finger pin assignments include pins 22-31. Do not flash it to the Arduino Uno used for the head.
- `Kreo 4K webcam` should usually be `ANUSHKA_CAMERA_INDEX=0`, but you can change it if another camera claims index `0`

## IPC notes

`EF.txt` files are module status/error outputs sent to the orchestrator.

`WS.txt` files are module command channels written by the orchestrator or other modules.

The refreshed runtime uses line-based messages instead of raw string concatenation, which avoids the old issue where multiple writes could merge into unreadable control data.

## Speech quality

The default `ANUSHKA_TTS_ENGINE=pyttsx3` works offline, but it can sound robotic because it depends on the operating system voice installed on the laptop.

For a more human voice, set:

```env
ANUSHKA_TTS_ENGINE=openai
ANUSHKA_TTS_MODEL=gpt-4o-mini-tts
ANUSHKA_TTS_VOICE=nova
ANUSHKA_TTS_INSTRUCTIONS=Speak warmly, naturally, and conversationally, like a friendly humanoid robot talking to visitors.
```

That requires `OPENAI_API_KEY`. The robot will still move the jaw and gestures while speaking.

## Current scope

The refreshed path/config/runtime layer is implemented for the active orchestration, hearing, speech, hands, eye, jaw, neck, base, heartbeat, and monitor modules.

The legacy vision stack remains available, but it is still the least-modernized part of the repo and should be validated on hardware before production demos.
# lucy_3.14
