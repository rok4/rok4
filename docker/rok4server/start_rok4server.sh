#!/bin/bash

# Apply default env var
printenv > /tmp/custom_env
source /etc/default/rok4server
source /tmp/custom_env


# Refactor some variable
export SERVICE_KEYWORDS_XML=$(echo $SERVICE_KEYWORDS | tr ',' '\n' | sed "s/^/<keyword>/g" | sed "s/$/<\/keyword>/g")
export SERVICE_FORMATLIST_XML=$(echo $SERVICE_FORMATLIST | tr ',' '\n' | sed "s/^/<format>/g" | sed "s/$/<\/format>/g")
export SERVICE_GLOBALCRSLIST_XML=$(echo $SERVICE_GLOBALCRSLIST | tr ',' '\n' | sed "s/^/<crs>/g" | sed "s/$/<\/crs>/g")


# Setup server.conf
eval "echo \"$(cat /etc/rok4/config/server.conf)\"" > /etc/rok4/config/server.conf

# Setup services.conf
eval "echo \"$(cat /etc/rok4/config/services.conf)\"" > /etc/rok4/config/services.conf

# Centralisation des descripteurs de couches
find /pyramids/ -maxdepth 2 -name "*.lay" -exec cp '{}' /layers/ \;

/bin/rok4 -f /etc/rok4/config/server.conf