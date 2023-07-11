#!/bin/sh
# Mount the project source code and enter the container

OCI_IMAGE_NAME_DEFAULT="${USER}/diagram-server"
OCI_IMAGE_NAME="${OCI_IMAGE_NAME:-${OCI_IMAGE_NAME_DEFAULT}}"
OCI_TOOL="${OCI_TOOL:-docker}"

usage()
{
	echo "usage: $0 path/to/project"
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

project="$1"

# Run the container interactively, this will drop us to a shell inside
# For debugging diagram-server, you might find mounting it useful
# --mount type=bind,source="$PWD",target=/root/diagram-server \
"$OCI_TOOL" run \
	--mount type=bind,source="${project}",target=/mnt \
	--publish 8080:8080 \
	-it "$OCI_IMAGE_NAME"
