# Spectacle - The KDE Screenshot Utility

Spectacle is a screenshot taking utility for the KDE desktop. Spectacle
can also be used in non-KDE X11 desktop environments.

## Contributing

Spectacle is developed under the KDE umbrella and uses KDE infrastructure
for development.

When using kdesrc-build to build Spectacle, and running without the
installed built session, built Spectacle will not be authorized to take a
screenshot and the application may not appear. To work around this, you can
set the environment variable `KWIN_SCREENSHOT_NO_PERMISSION_CHECKS=1`. Note
that this will authorize any process to take a screenshot of your desktop,
so your system's security will be impacted.

Please see the file CONTRIBUTING for details on coding style and how
to contribute patches. Please note that pull requests on GitHub aren't
supported. The recommended way of contributing patches is via KDE's
instance of GitLab at https://invent.kde.org/graphics/spectacle.

## Release Schedule

Spectacle is released by KDE's release service and has three
major releases every year. They are numbered YY.MM, where YY is the two-
digit year and MM is the two-digit month. Major releases are made in April,
August and December every year. The Spectacle version follows the KDE
release service version.

## Reporting Bugs

Please report bugs at KDE's Bugzilla, available at https://bugs.kde.org/

For discussions, the #kde-devel IRC channel and the kde-devel mailing list
are good places to post.

