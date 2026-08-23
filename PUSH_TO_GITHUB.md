# Push this package to a new GitHub repository

Create an empty repository first (do not add a README/license remotely), then from this
folder run:

```powershell
git init
git add .
git commit -m "Initial Smart Denoise P0-P2 handoff"
git branch -M main
git remote add origin https://github.com/<YOUR_USER>/<YOUR_NEW_REPO>.git
git push -u origin main
```

Suggested repository name:

```text
smart-denoise
```

After push, keep the first development phase focused on:

1. Windows x64 native JUCE build.
2. Fix any compiler differences against the selected JUCE tag.
3. Produce a VST3 + Standalone test artifact.
4. Listening tests before any P3 work.
