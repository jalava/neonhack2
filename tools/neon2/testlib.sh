# testlib.sh - helpers for the NeonHack 2 test scripts (sourced, not run).
# Needs $top (the source tree); drives the game through tools/nhdrive.
drive="$top/tools/nhdrive"
fails=0

screen() { "$drive" screen; }
keys() { "$drive" keys "$@"; }
key() { "$drive" key "$@"; }
# expect TEXT [what]: the screen must show TEXT
expect() {
    if screen | grep -qF -- "$1"; then
        echo "ok   ${2:-$1}"
    else
        echo "FAIL ${2:-$1}"; screen | sed 's/^/     | /'; fails=$((fails + 1))
    fi
}
# expect_msg TEXT [what]: TEXT shows up in the next few messages
expect_msg() {
    local i
    for i in 1 2 3 4 5 6; do
        if screen | grep -qF -- "$1"; then echo "ok   ${2:-$1}"; return; fi
        screen | grep -q -e '--More--' || break
        key Enter
    done
    echo "FAIL ${2:-$1}"; screen | sed 's/^/     | /'; fails=$((fails + 1))
}
# dismiss --More-- and menus until the map is back
clear_msgs() {
    local i
    for i in 1 2 3 4 5 6 7 8; do
        if screen | grep -q -e '--More--' -e '(end)' -e '([0-9]* of [0-9]*)'; then
            key Enter
        else
            break
        fi
    done
}
# listed TEXT [what]: #wizstory list (any page of it) shows TEXT
listed() {
    local i found=
    wizstory list
    for i in 1 2 3 4 5; do
        screen | grep -qF -- "$1" && { found=1; break; }
        screen | grep -q -e '--More--' -e '(end)' -e '([0-9]* of' || break
        key Enter
    done
    if [ -n "$found" ]; then echo "ok   ${2:-$1}"
    else echo "FAIL ${2:-$1}"; screen | sed 's/^/     | /'; fails=$((fails + 1)); fi
    clear_msgs
}
ext() { keys "#$1"; key Enter; }
wizstory() { ext wizstory; keys "$1"; key Enter; }
# talk NPC: stand next to it and #chat
talk() {
    local dir
    wizstory "goto $1"
    dir=$(screen | sed -n 's/.*Chat direction: \(.\)\..*/\1/p' | head -1)
    clear_msgs
    [ -z "$dir" ] && { echo "FAIL can't get next to $1"; fails=$((fails + 1)); return; }
    ext chat; keys "$dir"
}
levelport() { key C-v; keys "$1"; key Enter; clear_msgs; }

# restart: save the game, then start the game process again (restore)
save_and_restore() {
    keys S; keys y; sleep 1
    tmux kill-session -t "$NHSESSION" 2>/dev/null
    tl_home=$(mktemp -d /tmp/nhhome.XXXXXX)
    tmux new-session -d -s "$NHSESSION" -x 80 -y 24 \
        "cd '$top/playground' && HOME='$tl_home' TERM=xterm-256color NETHACKOPTIONS='$NHOPTS' ./neonhack2 -D; sleep 600"
    sleep 1.5; clear_msgs
    screen | grep -q 'keep the save file' && keys n
    clear_msgs
}
