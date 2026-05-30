# Set up the ESP-IDF environment (get_idf is an interactive shell alias and is not
# available in this non-interactive script, so source export.sh directly).
. "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh"
# Switch to BENCH mode: drop stale sdkconfig and chain in the bench override.
# -DSDKCONFIG_DEFAULTS replaces the auto-discovered defaults, so list the base file first.
rm -f sdkconfig
# idf.py monitor cannot log to a file itself, so tee its output. mkdir -p ensures
# the target directory exists. Exit the monitor with Ctrl-] once the run finishes.
mkdir -p data
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.bench" build \
    && idf.py flash \
    && idf.py monitor | tee "data/bench_$(date +%Y%m%d_%H%M%S).log"
