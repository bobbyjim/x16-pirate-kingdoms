
my @map;

my $rows = shift || 90;
my $cols = shift || 70;

my $mu = 2;
for (1..5 * $mu)
{
   my $x = int(rand()*($cols-10))+5;
   my $y = int(rand()*($rows-10))+5;

   for (1..int(rand($rows*$cols)/(2*$mu)))
   {
      $x += 2*rand()-1;
      $y += 2*rand()-1;

      next if $x<0 || $y<0 || $x>$cols || $y>$rows;
      #my $dx = $x + 10 * (rand()-rand());
      #my $dy = $y + 10 * (rand()-rand());
      #next if $dx<0 || $dy<0;

      $map[$x][$y]++ if $map[$dx][$dy] < 8;
   }
}

# saveTerrain(); # to MAP.BIN
showTerrain();

sub saveTerrain
{
   open my $out, '>', 'MAP.BIN';
   print $out pack 'xx';
#   print $out pack 'CC', $rows-1, $cols-1;
   for my $r (0..$rows-1)
   {
      for my $c (0..$cols-1)
      {
         #$map[$r][$c] |= 0;
         #$map[$r][$c] &= 0x0f;
         my $foo = $map[$r][$c] & 0x0f;

         $map[$r][$c] = int(rand(16))*16 + $foo if $map[$r][$c] > 15;
         print $out pack 'C', $map[$r][$c];

         print int($map[$r][$c]/16) . " " . ($map[$r][$c] & 15) . "\n" if $map[$r][$c] > 15;
      }
   }
   close $out;
}

sub showTerrain
{
   for my $r (0..$rows-1)
   {
      #print "DATA \"";
      for my $c (0..$cols-1)
      {
         $map[$r][$c] = ' ' unless defined $map[$r][$c];
         $map[$r][$c] = 9 if $map[$r][$c] > 9;
         print $map[$r][$c];
      }
      #print "\"\n";
      print "\n";
   }
   print "\n\n";
}
