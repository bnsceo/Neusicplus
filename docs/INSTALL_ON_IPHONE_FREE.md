# Install Neusicplus on your own iPhone for free

This uses Xcode's free Personal Team signing. The app is for personal testing and normally must be rebuilt/reinstalled every 7 days.

## What you need

- A Mac with the current Xcode installed
- Your Apple Account signed into Xcode
- Your iPhone and a USB cable for the first pairing
- Developer Mode enabled on the iPhone

## 1. Download the project

```bash
git clone https://github.com/bnsceo/Neusicplus.git
cd Neusicplus
git checkout feature/playable-daw-core
```

## 2. Generate and open the iOS Xcode project

```bash
chmod +x scripts/generate-ios-xcode.sh
./scripts/generate-ios-xcode.sh
```

## 3. Choose your free Personal Team

In Xcode:

1. Select the **Neusicplus** project in the navigator.
2. Select the **Neusicplus** target.
3. Open **Signing & Capabilities**.
4. Enable **Automatically manage signing**.
5. Choose your Apple Account's **Personal Team**.
6. Keep the bundle identifier unique. The default is `com.neusicwave.neusicplus`; append your initials if Xcode reports it is unavailable.

## 4. Connect the iPhone

1. Connect the iPhone to the Mac.
2. Tap **Trust** on the iPhone if prompted.
3. In Xcode, select the connected iPhone as the run destination.
4. If Xcode offers to register or prepare the device, accept it.

## 5. Enable Developer Mode

On the iPhone:

1. Open **Settings → Privacy & Security → Developer Mode**.
2. Turn it on and restart the iPhone.
3. After restart, confirm **Enable** and enter the device passcode.

The Developer Mode option may appear only after the iPhone has been paired with Xcode.

## 6. Install Neusicplus

1. Return to Xcode.
2. Select the **Neusicplus** scheme and your iPhone.
3. Press the **Run** button or use `⌘R`.
4. Accept the microphone permission when Neusicplus opens.

## Free-signing limitation

A Personal Team provisioning profile expires after 7 days. Reconnect the iPhone and press Run in Xcode again to reinstall or refresh the app.

## Current status

The iOS target and signing setup are included, but the playable DAW branch must pass native compilation before this installation path is considered ready for use.
