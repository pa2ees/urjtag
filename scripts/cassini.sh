#!/bin/bash -ex
pgrep xvcServer || xvcServer /dev/jtag_tesa &
wget -qN http://jenkins.imsar.us/job/cassini_mb/job/dev/lastSuccessfulBuild/artifact/output/cassini_1.svf
urj=cassini.urj
cat <<EOF > $urj
#!/usr/bin/jtag
cable xvc address=127.0.0.1
detect
svf cassini_1.svf
EOF
chmod +x $urj
$urj
rm $urj
#pkill xvcServer
