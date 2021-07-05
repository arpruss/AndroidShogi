CONVERT="inkscape --export-width=200"
DIR=../app/src/main/res/drawable/

animal() {
    echo $1 to animal_$2
    ${CONVERT} --export-png=${DIR}animal_$2.png $1.svg
}

animals() {
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
}

piece() {
    echo $2 to ${1}_$3
    ${CONVERT} --export-png=${DIR}${1}_$3.png $1/0$2.svg
}

piece_set() {
    piece $1 FU fu
    piece $1 GI gin
    piece $1 GY gyokusho
    piece $1 HI hi
    piece $1 KA kaku
    piece $1 KE kei
    piece $1 KI kin
    piece $1 KY kyo
    piece $1 NG nari_gin
    piece $1 NK nari_kei
    piece $1 NY nari_kyo
    piece $1 OU ou
    piece $1 RY ryu
    piece $1 TO to
    piece $1 UM uma
}

#animals
#piece_set international
#piece_set kanji_brown
#piece_set kanji_light
piece_set kanji_light_threedim
#piece_set kanji_red_wood
