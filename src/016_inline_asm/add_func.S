.text
.global get_min, get_product

get_min:
    CMP R0, R1
    MOVGT R0, R1
    BX LR


get_product:
    MUL R0, R0, R1
    BX LR
