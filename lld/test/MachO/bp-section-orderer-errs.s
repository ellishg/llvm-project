# RUN: not %lld -o /dev/null --compression-sort=malformed 2>&1 | FileCheck %s --check-prefix=COMPRESSION-MALFORM
# RUN: not %lld -o /dev/null --bp-compression-sort=malformed 2>&1 | FileCheck %s --check-prefix=COMPRESSION-MALFORM
# COMPRESSION-MALFORM: unknown value `malformed` for --bp-compression-sort=

# RUN: not %lld -o /dev/null '--bp-compression-sort-section=__TEXT*=x' 2>&1 | FileCheck %s --check-prefix=SECTION-LAYOUT-ERR
# RUN: not %lld -o /dev/null '--bp-compression-sort-section=__TEXT*=0=x' 2>&1 | FileCheck %s --check-prefix=SECTION-MATCH-ERR
# RUN: not %lld -o /dev/null '--bp-compression-sort-section=__TEXT*=0=0=0' 2>&1 | FileCheck %s --check-prefix=SECTION-EQ-ERR
# RUN: not %lld -o /dev/null '--bp-compression-sort-section=[' 2>&1 | FileCheck %s --check-prefix=SECTION-GLOB-ERR
# SECTION-LAYOUT-ERR: --bp-compression-sort-section: expected integer for layout_priority, got 'x'
# SECTION-MATCH-ERR: --bp-compression-sort-section: expected integer for match_priority, got 'x'
# SECTION-EQ-ERR: --bp-compression-sort-section: too many '=' in '__TEXT*=0=0=0'
# SECTION-GLOB-ERR: --bp-compression-sort-section: invalid glob pattern, unmatched '['

# RUN: not %lld -o /dev/null --compression-sort-startup-functions 2>&1 | FileCheck %s --check-prefix=STARTUP
# RUN: not %lld -o /dev/null --bp-compression-sort-startup-functions 2>&1 | FileCheck %s --check-prefix=STARTUP
# STARTUP: --bp-compression-sort-startup-functions must be used with --bp-startup-sort=function

# RUN: not %lld -o /dev/null --bp-startup-sort=function 2>&1 | FileCheck %s --check-prefix=STARTUP-COMPRESSION
# STARTUP-COMPRESSION: --bp-startup-sort=function must be used with --irpgo-profile
