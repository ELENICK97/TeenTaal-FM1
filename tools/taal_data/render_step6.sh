#!/bin/bash
# Step 6 deliverables: renders/step-6/ (build-host/render: the engine, the firmware's audio path). Sa A3.
#   tools/taal_data/render_step6.sh
set -e
cd "$(dirname "$0")/../.."
make -s build-host/render
D=renders/step-6
mkdir -p $D/1_medium $D/2_slow_fast $D/3_changes
R=build-host/render
dur() { echo "$1 * $2 * 60 / $3 + 1.5" | bc -l; }       # cycles matras mpm
i=0
while IFS='|' read -r name matras mpm; do
    i=$((i + 1))
    $R $(printf "$D/1_medium/%02d_%s_%d_mpm.wav" $i $name $mpm) $(dur 3 $matras $mpm) \
        0:sa=A3 0:taal.taal=$name 0:taal.mpm=$mpm 0.05:taal.on=1 | sed 's/, tone.*//'
done <<'L'
Teentaal|16|160
Ektaal|12|120
Jhaptaal|10|140
Rupak|7|140
Keherwa|8|200
Dadra|6|180
Jhoomra|14|60
Deepchandi|14|120
Dhamar|14|120
Chautaal|12|120
L
$R $D/2_slow_fast/teentaal_vilambit_40_mpm.wav $(dur 1 16 40) 0:sa=A3 0:taal.mpm=40 0.05:taal.on=1 | sed 's/, tone.*//'
$R $D/2_slow_fast/teentaal_drut_480_mpm.wav $(dur 6 16 480) 0:sa=A3 0:taal.mpm=480 0.05:taal.on=1 | sed 's/, tone.*//'
$R $D/2_slow_fast/ektaal_vilambit_40_mpm.wav $(dur 1 12 40) 0:sa=A3 0:taal.taal=Ektaal 0:taal.mpm=40 0.05:taal.on=1 | sed 's/, tone.*//'
$R $D/2_slow_fast/teentaal_160_with_tanpura.wav 26 0:sa=A3 0:tan.on=1 0:taal.mpm=160 2:taal.on=1 | sed 's/, tone.*//'
# tempo: 100 -> 160 -> 240 -> 320 -> 120 in steps; a smooth acceleration (1 mpm every 0.1 s); taal changes
$R $D/3_changes/teentaal_tempo_steps.wav 24 0:sa=A3 0:taal.mpm=100 0.05:taal.on=1 6:taal.mpm=160 11:taal.mpm=240 \
    15:taal.mpm=320 19:taal.mpm=120 | sed 's/, tone.*//'
EV=(0:sa=A3 0:taal.mpm=100 0.05:taal.on=1)
for k in $(seq 1 150); do EV+=("$(echo "2 + $k * 0.1" | bc):taal.mpm=$((100 + k))"); done
$R $D/3_changes/teentaal_accelerando_100_to_250.wav 20 "${EV[@]}" | sed 's/, tone.*//'
$R $D/3_changes/taal_changes.wav 22 0:sa=A3 0:taal.mpm=150 0.05:taal.on=1 4.5:taal.taal=Ektaal 9.5:taal.taal=Rupak \
    14:taal.taal=Keherwa 18:taal.on=0 | sed 's/, tone.*//'
$R $D/3_changes/human_0_then_100.wav 16 0:sa=A3 0:taal.mpm=160 0:taal.human=0 0.05:taal.on=1 8:taal.human=100 | sed 's/, tone.*//'
