#!/usr/bin/env perl
#
#  Takes a PNG file as an argument, sends it through
#  bin2c.py, and reformats the output into BASIC DATA
#  statements.  The data is runlength encoded as follows:
#
#  RL,[Value List]
#
#  If RL < 128, then [Value List] is simply a list of 
#  [RL] values.
#  
#  If RL > 128, then [Value List] is a single value 
#  that is repeated [RL-128] times.
#
#  RL = 128 is reserved.
#

#
#  Uncomment this line if you want line numbers.
#
#my $line = 10000;


my $pngfile = shift || die "SYNOPSIS: $0 [PNG file] [sprite BASIC file]\n";
my $outfile = shift || die "SYNOPSIS: $0 [PNG file] [sprite BASIC file]\n";
my $tmpfile = "$pngfile.tmp";

print STDERR "creating [$tmpfile]\n";
my $ret = system './png2sprite.py', $pngfile, $tmpfile;

die "ERROR invoking png2sprite.py on [$pngfile]\n" if $ret;

open my $in, '<', $tmpfile || die "ERROR: could not open tempfile '$tmpfile'\n";


my $mode   = 0;
my @out    = ();
my $buffer = [];
my %palette = ();

for(<$in>)
{
   next unless /0x/;
   chomp;
   s/0x/\$/g;
   s/,$//;

   # some slight optimizations
   s/\$00/0/g;

   my @numbers = split /,/;
   #print STDERR @numbers, "\n";

   for my $value (@numbers)
   {
      $palette{$value}++;

      if (@$buffer == 0)
      {
         $mode = 0;
         $buffer = [$mode, $value];
      }
      elsif ($mode == 0) # single values
      {
         if ($value ne $buffer->[-1])  	# good
         {
            push @$buffer, $value;
         }
         else # switch mode
         {
            $mode = 1;		 	# switch mode
            pop @$buffer;	 	# adjust buffer
            push @out, $buffer; 	# store old buffer
            $buffer = [$mode, $value, $value]; # and re-start buffer
         }
      }
      else # mode == 1, a run of 1 value
      {
         if ($value eq $buffer->[-1]) 	# good
         {
            push @$buffer, $value;
         }
         else # switch mode
         {
            $mode = 0;		 	# switch mode
            push @out, $buffer; 	# store old buffer
            $buffer = [$mode, $value];  # and re-start buffer
         }
      }
   }
}
# what's left in $buffer?
push @out, $buffer;


close $in;
unlink $tmpfile;

#print "palette: ", scalar keys(%palette), "entries\n";
#foreach my $key (sort keys %palette)
#{
#   print "$key ($palette{$key})\n";
#}

#
#  now flatten the list
#
my @flat = ();

foreach my $aref (@out)
{
   my @array = @$aref;
   my $mode = shift @array;
   my $count = @array;

   if ($mode == 1) # a run of 1 value
   {
      $count += 128;
      push @flat, $count, $array[0];
      #print STDERR "mode 1: $count, $array[0]\n";
   }
   else # mode == 0, a group of N values
   {
      push @flat, $count, @array if @array;
      #print STDERR "mode 0: $count, @array\n" if @array;
   }
}

#
#  now pretty print it
#
my ($name) = $pngfile =~ /^(.*)\.png/;
open my $fp, '>', $outfile;

printLine( "rem ---------------------------------------" );
printLine( "REM subroutine $pngfile" );
printLine( "rem ta = target address" );
printLine( "rem ---------------------------------------" );
printLine( "{:read-$name}" );
printLine( "? \"loading $name\"" );
printLine( "i=0" );
printLine( "{:loop-$name}" );
printLine( "if i/100=int(i/100) then ? int(i*100/4096) \" \\x9d%\\x91\"" );
printLine( "read rl :if rl<0 then return" );
printLine( "if rl<128 goto {:$name-values}" );
printLine( "rl=rl-128" );
printLine( "read x" );
printLine( "for y=1 to rl" );
printLine( "   vpoke \$0,ta+i,x" );
printLine( "   i=i+1" );
printLine( "next" );
printLine( "goto {:loop-$name}" );
printLine( "{:$name-values}" );
printLine( "for y=1 to rl" );
printLine( "   read x" );
printLine( "   vpoke \$0,ta+i,x" );
printLine( "   i=i+1" );
printLine( "next" );
printLine( "goto {:loop-$name}" );

{
   print $fp "$line " if defined $line;
   $line += 5 if defined $line;

   print $fp "DATA ";
   my $siz = 15;
   $siz = scalar(@flat)-1 if @flat < 16;
   print $fp join ',', @flat[0..$siz];
   print $fp "\n";

   splice @flat, 0, 16; 
   redo if @flat;
}
printLine( "DATA -1" );

close $fp;

#
#  prints with optional line numbering
#
sub printLine
{
   my $code = shift;
   print $fp "$line " if defined $line;
   $line += 5 if defined $line;
   print $fp uc "$code\n";
}


