
my @map;

my $rows = shift || 64;
my $cols = shift || 64;
my $sea_level = shift;
$sea_level = 4 unless defined $sea_level;
$sea_level = 0 if $sea_level < 0;
$sea_level = 15 if $sea_level > 15;

my $mu = 2;
my $coverage_bias = 1 + 0.5 * ($sea_level - 3);
$coverage_bias = 0.5 if $coverage_bias < 0.5;
my $seed_count = int((5 * $mu) * $coverage_bias + 0.5);

for (1..$seed_count)
{
   my $x = int(rand()*($cols-10))+6;
   my $y = int(rand()*($rows-10))+6;

   for (1..int(rand($rows*$cols*$coverage_bias)/($mu)))
   {
      $x += 2*rand()-1;
      $y += 2*rand()-1;

      next if $x<0 || $y<0 || $x>=$cols || $y>=$rows;
      $map[$x][$y]++;
   }
}

saveTerrain(); # to MAP.BIN
showTerrain();
showHistogram();
showSeaLevelSummary();

sub saveTerrain
{
   open my $out, '>', 'MAP.BIN';
   print $out pack 'xx';
   for my $r (0..$rows-1)
   {
      for my $c (0..$cols-1)
      {
         my $v = $map[$r][$c] || 0;
         $v = 15 if $v > 15;
         print $out pack 'C', $v;
      }
   }
   close $out;
}

sub showTerrain
{
   my @hex = (0..9, 'a'..'f');
   for my $r (0..$rows-1)
   {
      for my $c (0..$cols-1)
      {
         my $v = $map[$r][$c] || 0;
         $v = 15 if $v > 15;
         print '_' if $v == 0; # seafloor
         print ' ' if $v == 1; # deep water
         print '.' if $v == 2; # shallow water
         print '~' if $v == 3; # shore
         print $hex[$v] if $v >= 4; # land, with increasing elevation
      }
      print "\n";
   }
   print "\n";
}

sub showHistogram
{
   my @count = (0) x 16;
   for my $r (0..$rows-1)
   {
      for my $c (0..$cols-1)
      {
         my $v = $map[$r][$c] || 0;
         $v = 15 if $v > 15;
         $count[$v]++;
      }
   }
   my $total = $rows * $cols;
   print "Value distribution (total cells: $total):\n";
   my @hex = (0..9, 'a'..'f');
   for my $v (0..15)
   {
      my $bar = '#' x int($count[$v] * 40 / $total);
      printf "  %s (%4d, %5.1f%%)  %s\n", $hex[$v], $count[$v], 100*$count[$v]/$total, $bar;
   }
   print "\n";
}

sub showSeaLevelSummary
{
   my ($ocean, $shore, $land) = (0, 0, 0);
   for my $r (0..$rows-1)
   {
      for my $c (0..$cols-1)
      {
         my $v = $map[$r][$c] || 0;
         $v = 15 if $v > 15;
         if ($v < $sea_level) {
            $ocean++;
         } elsif ($v == $sea_level) {
            $shore++;
         } else {
            $land++;
         }
      }
   }

   my $total = $rows * $cols;
   print "Sea level threshold: $sea_level\n";
   printf "  ocean (< %d): %4d (%5.1f%%)\n", $sea_level, $ocean, 100*$ocean/$total;
   printf "  shore (= %d): %4d (%5.1f%%)\n", $sea_level, $shore, 100*$shore/$total;
   printf "  land  (> %d): %4d (%5.1f%%)\n", $sea_level, $land, 100*$land/$total;
   print "\n";
}
