CONVERT="inkscape --export-width=200"
DIR=../app/src/main/res/drawable/

animal() {
    echo $1 to animal_$2
    ${CONVERT} --export-png=${DIR}animal_$2.png $1.svg
}
animal chick fu
animal red_chick to
animal lion ou
animal chick fu
animal giraffe hi
animal red_giraffe ryu
animal elephant kaku
animal red_elephant uma
animal dog kin
animal cat gin
animal red_cat nari_gin
animal bunny kei
animal red_bunny nari_kei
animal pig kyo
animal red_pig nari_kyo
