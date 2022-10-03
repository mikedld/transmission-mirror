#!/bin/sh

# Generate files to be included: only overwrite them if changed so make
# won't rebuild everything unless necessary
replace_if_differs() {
  if cmp "$1" "$2" > /dev/null 2>&1; then
    rm -f "$1"
  else
    mv -f "$1" "$2"
  fi
}

echo "creating libtransmission/version.h"

user_agent_prefix=$(grep 'set[(]TR_USER_AGENT_PREFIX' CMakeLists.txt | cut -d \" -f 2)

peer_id_prefix=$(grep 'set[(]TR_PEER_ID_PREFIX' CMakeLists.txt | cut -d \" -f 2)

major_version=$(echo "${user_agent_prefix}" | awk -F . '{print $1}')
minor_version=$(echo "${user_agent_prefix}" | awk -F . '{print $2}')
patch_version=$(echo "${user_agent_prefix}" | awk -F . '{print $3}')

vcs_revision=
vcs_revision_file=REVISION

if [ -n "$JENKINS_URL" ] && [ -n "$GIT_COMMIT" ]; then
  vcs_revision=$GIT_COMMIT
elif [ -n "$TEAMCITY_PROJECT_NAME" ] && [ -n "$BUILD_VCS_NUMBER" ]; then
  vcs_revision=$BUILD_VCS_NUMBER
elif [ -e ".git" ] && type git > /dev/null 2>&1; then
  vcs_revision=$(git rev-list --max-count=1 HEAD)
elif [ -f "$vcs_revision_file" ]; then
  vcs_revision=$(cat "$vcs_revision_file")
fi

if [ -n "$vcs_revision" ]; then
  [ -f "$vcs_revision_file" ] && [ "$(cat "$vcs_revision_file")" = "$vcs_revision" ] || echo "$vcs_revision" > "$vcs_revision_file"
else
  vcs_revision=0
  rm -f "$vcs_revision_file"
fi

vcs_revision=$(echo $vcs_revision | head -c10)

cat > libtransmission/version.h.new << EOF
#pragma once

#define PEERID_PREFIX             "${peer_id_prefix}"
#define USERAGENT_PREFIX          "${user_agent_prefix}"
#define VCS_REVISION              "${vcs_revision}"
#define VCS_REVISION_NUM          ${vcs_revision}
#define SHORT_VERSION_STRING      "${user_agent_prefix}"
#define LONG_VERSION_STRING       "${user_agent_prefix} (${vcs_revision})"
#define VERSION_STRING_INFOPLIST  ${user_agent_prefix}
#define BUILD_STRING_INFOPLIST    14714.${major_version}.${minor_version}.${patch_version}
#define MAJOR_VERSION             ${major_version}
#define MINOR_VERSION             ${minor_version}
#define PATCH_VERSION             ${patch_version}
EOF

# Add a release definition
case "${peer_id_prefix}" in
  *X-) echo '#define TR_BETA_RELEASE           1' ;;
  *Z-) echo '#define TR_NIGHTLY_RELEASE        1' ;;
  *)   echo '#define TR_STABLE_RELEASE         1' ;;
esac >> "libtransmission/version.h.new"

replace_if_differs libtransmission/version.h.new libtransmission/version.h
