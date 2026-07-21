# Install Neusicplus on your iPhone 12 for free

This uses Xcode's free Personal Team signing. The app is for personal testing and normally must be rebuilt and reinstalled every 7 days.

## Why Developer Mode may be missing

On iPhone, **Developer Mode usually does not appear until the phone has first been paired with a Mac running Xcode**. Do not expect to find it before completing the pairing steps below.

## What you need

- A Mac with the current Xcode installed
- Your Apple Account signed into Xcode
- Your iPhone 12
- A Lightning-to-USB cable for the first pairing
- An iPhone passcode enabled

## 1. Install and open Xcode

Install Xcode from the Mac App Store, then open it once and allow it to install any required components.

In Xcode, open **Xcode → Settings → Accounts**, press **+**, and sign in with the same Apple Account you want to use for the free Personal Team.

## 2. Download Neusicplus

```bash
git clone https://github.com/bnsceo/Neusicplus.git
cd Neusicplus
git checkout feature/playable-daw-core
```

## 3. Generate the iOS Xcode project

```bash
chmod +x scripts/generate-ios-xcode.sh
./scripts/generate-ios-xcode.sh
```

The script opens the generated Xcode project.

## 4. Pair the iPhone 12 with Xcode first

1. Unlock the iPhone 12.
2. Connect it to the Mac with the Lightning cable.
3. Tap **Trust** on the iPhone if **Trust This Computer?** appears.
4. Enter the iPhone passcode.
5. In Xcode, open **Window → Devices and Simulators**.
6. Select the iPhone 12 in the left sidebar.
7. Click **Pair**, **Use for Development**, or **Prepare Device** if Xcode shows one of those buttons.
8. Leave the phone connected until Xcode finishes preparing it.

## 5. Turn on Developer Mode

After pairing has started, check the iPhone again:

1. Open **Settings**.
2. Open **Privacy & Security**.
3. Scroll to the bottom of the Security section.
4. Tap **Developer Mode**.
5. Turn it on.
6. Tap **Restart**.
7. After the iPhone restarts, unlock it.
8. Tap **Enable** in the confirmation dialog.
9. Enter the iPhone passcode.

### If Developer Mode is still missing

- Confirm the iPhone is running iOS 16 or later.
- Disconnect and reconnect the cable.
- Unlock the iPhone before opening **Window → Devices and Simulators**.
- In Xcode, select the iPhone and wait until preparation completes.
- Restart both the Mac and iPhone, reconnect, and repeat the pairing step.
- Developer Mode appears only after Xcode initiates pairing or the phone has previously been paired with a Mac.

## 6. Choose the free Personal Team

In Xcode:

1. Select the **Neusicplus** project in the navigator.
2. Select the **Neusicplus** target.
3. Open **Signing & Capabilities**.
4. Enable **Automatically manage signing**.
5. Choose your Apple Account's **Personal Team**.
6. If Xcode says the bundle identifier is unavailable, change it to something unique, for example:

```text
com.andersonpaulino.neusicplus
```

## 7. Install Neusicplus

1. In the Xcode toolbar, choose the **Neusicplus** scheme.
2. Choose your connected **iPhone 12** as the run destination.
3. Press the triangular **Run** button or press `Command-R`.
4. Keep the iPhone unlocked while Xcode builds and installs the app.
5. Accept the microphone permission when Neusicplus first opens.

## If iPhone says the developer is not trusted

For a free Personal Team build, iOS may ask you to trust the development certificate:

1. Open **Settings → General → VPN & Device Management**.
2. Tap the profile under **Developer App**.
3. Tap **Trust**.
4. Return to Neusicplus.

## Free-signing limitation

A Personal Team provisioning profile expires after 7 days. Reconnect the iPhone and press **Run** in Xcode again to reinstall or refresh the app.

## Current status

The repository now contains the iOS target, simulator build verification, signing setup, and iPhone 12 instructions. Do not attempt the installation until the macOS and iOS simulator builds pass successfully on GitHub Actions.
