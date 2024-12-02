set -e 

(
  cd "$(dirname "$0")" 
  gcc app/*.c -o /tmp/shell-target
)

# Copied from codecrafters
exec /tmp/shell-target "$@"
