echo "clean outputs..."
if (Test-Path "./out/") {
    rm -r ./out/
}
$null = mkdir ./out/

$main = "main.c"
$hex = "./out/main.hex"
$bin = "./out/main.bin"

$iram = 256
$xram = 4096
$code = 0x8000

$sources = ls *.c
if ($sources -ne $null) {
    echo "compiling sources..."
    foreach ($s in $sources) {
        if ($s -match $main) {
            continue
        }
        echo "CC: $s"
        sdcc -mmcs51 --model-large --opt-code-size --iram-size $iram --xram-size $xram --code-size $code -c $s -o ./out/
    }
    
    $objs = ls ./out/*.rel
    if ($objs -ne $null) {
        echo "linking objects..."
        sdcc -mmcs51 --model-large --opt-code-size --iram-size $iram --xram-size $xram --code-size $code $main $objs -o $hex
        echo "making binary..."
        makebin $hex $bin

        echo "hex file at: $hex"
        echo "bin file at: $bin"
    }
}
