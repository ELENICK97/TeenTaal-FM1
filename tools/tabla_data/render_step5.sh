#!/bin/bash
# Step 5 deliverables: renders/step-5/ (tools/render/tabla_demo: the engine, the firmware's audio path).
#   tools/tabla_data/render_step5.sh
set -e
cd "$(dirname "$0")/../.."
make -s build-host/tabla_demo
D=renders/step-5
rm -f $D/1_strokes/*.wav $D/2_bols/*.wav $D/3_tunings/*.wav
mkdir -p $D/1_strokes $D/2_bols $D/3_tunings $D/4_AB_compression $D/5_phrases
T=build-host/tabla_demo
A3=-300
# 1. every stroke: its takes one after the other (dynamic 100), then one take at 40, 80, 127
i=0
for s in "Na NA 2" "Tin TIN 1" "Tun TUN 2" "Te TE 2" "Ti TI 1" "Ne NE 1" "Re RE 1" "Tas TAS 2" "Dhet DHEC 1" \
         "Ge GE 5" "Meend GEM 2" "Ke KE 3"; do
    set -- $s; i=$((i + 1)); ev=(); t=0.2
    for k in $(seq $3); do ev+=("$t:$1/100"); t=$(echo "$t + 1.3" | bc); done
    for d in 40 80 127; do ev+=("$t:$1/$d"); t=$(echo "$t + 1.3" | bc); done
    $T $(printf "$D/1_strokes/%02d_%s.wav" $i $2) $(echo "$t + 0.8" | bc) sa=$A3 "${ev[@]}"
done
# 2. every bol at three dynamics, Sa A3
i=0
for b in $($T -list | awk '{print $1}'); do
    i=$((i + 1))
    $T $(printf "$D/2_bols/%02d_%s.wav" $i $b) 4.4 sa=$A3 0.2:$b/40 1.6:$b/80 3.0:$b/127
done
# 3. the dayan at several Sa: G3 (G flute), A3 (A flute, the default), C4, E4 (D flute: Sa D4 folds to D4)
for s in "G3 -500" "A3 -300" "C4 0" "D4 200" "E4 400" "Fs3_top_of_window -600"; do
    set -- $s
    $T $D/3_tunings/dayan_Sa_$1.wav 8.6 sa=$2 0.2:Na 1.5:Tin 2.8:Tun 4.1:Dha 5.4:Dhin 6.7:Te 6.9:Re 7.1:Te 7.3:Na
done
# 5. phrases (hand-placed: the taal engine is step 6), with and without the tanpura
P=(0.2:Dha/120 0.5:Dhin 0.8:Dhin 1.1:Dha 1.4:Dha 1.7:Dhin 2.0:Dhin 2.3:Dha 2.6:Dha/110 2.9:Tin 3.2:Tin 3.5:Ta
   3.8:Ta 4.1:Dhin 4.4:Dhin 4.7:Dha 5.0:Dha/120 5.3:Dhin 5.6:Dhin 5.9:Dha)
$T $D/5_phrases/01_teentaal_theka_by_hand_A3.wav 7.5 sa=$A3 "${P[@]}"
$T $D/5_phrases/02_teentaal_theka_by_hand_A3_with_tanpura.wav 7.5 sa=$A3 tan=1 "${P[@]}"
Q=(); t=0.2; k=0
for b in Dha Ti Ra Ki Ta Ta Ka Ti Ra Ki Ta Dha Ge Na Dha Ge Na Dha Tin Na Ke Na Dha; do
    k=$((k + 1)); Q+=("$t:$b/$((70 + (k * 37) % 40))"); t=$(echo "$t + 0.125" | bc)
done
$T $D/5_phrases/03_fast_phrase_8_per_s_A3.wav 4 sa=$A3 "${Q[@]}"
~/.venvs/teentaal/bin/python tools/tabla_data/ab_compression.py $D/4_AB_compression
