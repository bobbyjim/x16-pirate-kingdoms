
srand();

open my $out, '>', 'MAP.BIN';

print $out pack 'xx';  # two nulls

for my $i (0..1023)
{
    print $out pack 'C', int(rand()*8);  # 1st byte = random 8 tiles
    print $out pack 'x';                 # nothing in the 2nd byte
}

close $out;
