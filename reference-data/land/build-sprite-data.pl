use strict;

foreach my $filename (<*.png>)
{
   my ($thing) = $filename =~ /^(.*)\.png/;
   $thing =~ s/ /-/;
   $thing = "spr-$thing.list";

   print "writing $thing\n";

   system './c2rl.pl', $filename, $thing;
}
