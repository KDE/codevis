#!/bin/bash
# Use: ./dbustest.sh /path/to/build /path/to/db

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 /path/to/build /path/to/db" 1>&2
    exit 1
fi

EXEC="$1/desktopapp/codevis"
DB="$2"

if ! [ -x "$EXEC" ]; then
    echo "$EXEC is not a build directory" 1>&2
    exit 1
fi

if ! [ -f "$DB" ]; then
    echo "$DB not found" 1>&2
    exit 1
fi

echo "Backing up configuration"
mv ~/.config/Codethink/DiagramViewer.conf ~/.config/Codethink/DiagramViewer.conf-bkp > /dev/null 2>&1
mv ~/.config/DiagramViewerrc ~/.config/DiagramViewerrc-bkp > /dev/null 2>&1

# We store the logs here.
# lakosdiagrma.log is the application log
# timed_codevis is the dbus log, with all the time information for each call.

rm codevis.log > /dev/null 2>&1
rm timed_codevis.log > /dev/null 2>&1

function cleanup {
  echo "Restoring configuration"
  mv ~/.config/Codethink/DiagramViewer.conf-bkp ~/.config/Codethink/DiagramViewer.conf > /dev/null 2>&1
  mv ~/.config/DiagramViewerrc-bkp ~/.config/DiagramViewerrc > /dev/null 2>&1
}

trap cleanup EXIT

export DEFAULT_SLEEP=0.5s
export TEE_CMD="tee -a timed_codevis.log"

# Launch the app in background.
"$EXEC" > codevis.log 2>&1 &

# show the application pid
echo "$!"

# We have a lot of required word splitting, so let's silence shellcheck

# Wait a bit for the application to launch
sleep 2s

# For all the possible methods, see codevisdbusinterface.h

export DBUSCALL="dbus-send --print-reply --dest=org.codethink.CodeVis /Application org.codethink.CodeVis"

# Loads a database file.
echo "Loading Database" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadDatabase string:"$DB" >> timed_codevis.log 2>&1

# Creates a new tab
echo "Creating new tab" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestNewTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Closes the tab.
echo "Closing created tab" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestCloseCurrentTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Opens the split view.
echo "Open Split View" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestToggleSplitView >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

echo "Selecting first split" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestSelectLeftSplitView >> timed_codevis.log 2>&1

# Creates a new tab
echo "Open new tab on the first split" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestNewTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Closes the tab.
echo "Closing the tab"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestCloseCurrentTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# shellcheck disable=2086
$DBUSCALL.requestSelectRightSplitView >> timed_codevis.log 2>&1

# Creates a new tab
echo "Open tab on the second split"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestNewTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Closes the tab.
echo "Close the tab on the second split"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestCloseCurrentTab >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Closes the split view.
echo "Closing split view" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestToggleSplitView >> timed_codevis.log 2>&1
sleep "$DEFAULT_SLEEP"

# Loads things from a single package, from the topmost, down to a class.
# Try to load a Package Group, BAL.
# loadEntity loads a specific, fully qualified name, entity.
echo "Loading package group BAL"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadPackage string:"bal" >> timed_codevis.log 2>&1
sleep 2

# Try to load a Package, balb.
echo "Loading package BALB"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadPackage string:"balb" >> timed_codevis.log 2>&1
sleep 2

# Try to load a Package, component.
echo "Loading component balb_controlmanager"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadComponent string:"balb_controlmanager" >> timed_codevis.log 2>&1
sleep 2

# Try to load a class..
echo "Loading class BloombergLP::balb::ControlManager::ControlHandler"  | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadClass string:"BloombergLP::balb::ControlManager::ControlHandler" >> timed_codevis.log 2>&1
sleep 2

# Load another package group.
echo "Loading package group bbl" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestLoadPackage string:"bbl" >> timed_codevis.log 2>&1
sleep 2

# Quits
echo "Finalizing" | $TEE_CMD
# shellcheck disable=2086
$DBUSCALL.requestQuit  >> timed_codevis.log 2>&1
