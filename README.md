# Monocle for Hyprland

This is a simple monocle "tabbed layout" for hyprland. It is basically a simple
script that creates a tabbed window group and adds every window on the current
workspace to that group. I made this because the builtin fullscreen mode with
hyprland is not ergonomic to cycle through (it jumps around and triggers window
resizes).

See https://github.com/hyprwm/Hyprland/issues/2605 and
https://github.com/hyprwm/Hyprland/issues/2822.

# Installation

## Manual

Simply clone the repo and run `make install`. This will build the `monocle.so`
file and install it to `$HOME/.local/share/hyprload/plugins/bin`. You can then
load it using the hyprload plugin manager or manually by adding this line to
your config:

```
exec-once = hyprctl plugin load ~/.local/share/hyprload/plugins/bin/monocle.so
```

Be warned that loading plugins can cause hyprland to crash, so you should test
loading the .so in a nested session if you can to make sure everything works as
expected before adding this line to your config.

## Hyprpm

As you can see, this repo does have a hyprpm.toml file, but I couldn't get
hyprpm to work so I haven't tested that. If it works great, if not I won't
likely be fixing it myself but a PR would be welcome :).

# Usage

The plugin creates three dispatchers for hyprctl: `monocle:on`, `monocle:off`,
and `monocle:toggle`. You can just bind any of these in your config. I use the
following binding:

```
bind = $mainMod, M, monocle:toggle
```
