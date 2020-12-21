####
Suil
####

Suil is a library for loading and wrapping LV2 plugin UIs.

With Suil,
a host written in one supported toolkit can embed a plugin UI written in another.
Suil insulates hosts from toolkit libraries used by plugin UIs.
For example,
a Gtk host can embed a Qt UI without linking against Qt at compile time.

Suil also handles the embedding of native platform UIs (which are recommended) in common toolkits,
for example embedding an X11 UI in a Gtk host.
