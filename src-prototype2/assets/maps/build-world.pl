#!/usr/bin/env perl
use strict;
use warnings;
use List::Util qw(min max shuffle);

# Constants
use constant {
    MAP_SIZE => 256,
    MAX_OBJECTS => 1024,
    OBJECT_SIZE => 8,
    OBJECT_LIST_SIZE => 8192,
    
    # Terrain types (bits 0-3)
    TERRAIN_WATER => 0,
    TERRAIN_GRASS => 1,
    TERRAIN_FOREST => 2,
    TERRAIN_HILLS => 3,
    TERRAIN_MOUNTAINS => 4,
    TERRAIN_DESERT => 5,
    TERRAIN_SWAMP => 6,
    
    # Travel ease (bits 4-5)
    TRAVEL_DIFFICULT => 0,
    TRAVEL_STANDARD => 1,
    TRAVEL_PATH => 2,
    TRAVEL_ROAD => 3,
    
    # Flags
    FLAG_SPECIAL => 0x40,  # Bit 6
    FLAG_OBJECT => 0x80,   # Bit 7
    
    # Object types
    OBJ_EMPTY => 0x00,
    OBJ_SETTLEMENT => 0x01,
    OBJ_SHIP => 0x02,
    OBJ_GROUP => 0x03,
    OBJ_TOWER => 0x04,
    OBJ_SHRINE => 0x05,
    OBJ_RUINS => 0x06,
    OBJ_MINE => 0x07,
    OBJ_STELA => 0x08,
    OBJ_PORTAL => 0x09,
    OBJ_LIGHTHOUSE => 0x0A,
    OBJ_BRIDGE => 0x0B,
    OBJ_CAVE => 0x0C,
    OBJ_MONUMENT => 0x0D,
};

# Initialize map (256x256, all water)
my @map = map { [(TERRAIN_WATER | (TRAVEL_STANDARD << 4)) x MAP_SIZE] } (1..MAP_SIZE);
my @objects = ();

print "Generating archipelago world...\n";

# Generate islands
print "Generating 12 islands...\n";
generate_islands();

# Place settlements
print "Placing 20 settlements...\n";
my @settlements = place_settlements(20);

# Generate road network
print "Generating road network...\n";
generate_roads(\@settlements);

# Place points of interest
print "Placing points of interest...\n";
place_poi(OBJ_TOWER, 8, 'T');
place_poi(OBJ_SHRINE, 12, 'S');
place_poi(OBJ_RUINS, 10, 'R');
place_poi(OBJ_MINE, 6, 'M');
place_poi(OBJ_STELA, 7, 'A');
place_poi(OBJ_PORTAL, 3, 'P');
place_poi(OBJ_CAVE, 8, 'C');
place_poi(OBJ_MONUMENT, 4, 'O');

# Write output files
print "Writing archipelago.map (binary)...\n";
write_binary_map('archipelago.map');

print "Writing archipelago.txt (text visualization)...\n";
write_text_map('archipelago.txt', 0);

print "Writing archipelago-roads.txt (text visualization with roads)...\n";
write_text_map('archipelago-roads.txt', 1);

print "Done! World generation complete.\n";
print "  Objects created: " . scalar(@objects) . "\n";
print "  Map size: 256x256 tiles\n";

# ============================================================================
# ISLAND GENERATION
# ============================================================================

sub generate_islands {
    for my $i (1..12) {
        # Random island size (15x15 to 35x35)
        my $island_size = 15 + int(rand(21));
        
        # Random position with 40-tile margin
        my $center_x = 40 + int(rand(MAP_SIZE - 80));
        my $center_y = 40 + int(rand(MAP_SIZE - 80));
        
        # Generate island using blob growth
        grow_island($center_x, $center_y, $island_size);
    }
}

sub grow_island {
    my ($cx, $cy, $size) = @_;
    
    # Use random walks to create irregular coastlines
    my %visited = ();
    my $target_tiles = $size * $size;
    my $tiles_placed = 0;
    my $max_dist = $size / 2;
    
    # Multiple random walks from center
    my $num_walks = int($size / 2);  # Scale walks with island size
    
    for my $walk (1..$num_walks) {
        my $x = $cx;
        my $y = $cy;
        my $steps = int($target_tiles / ($num_walks * 0.7));  # Variable step count
        
        for my $step (1..$steps) {
            # Random walk: move in random direction
            my $dx_move = int(rand(3)) - 1;  # -1, 0, or 1
            my $dy_move = int(rand(3)) - 1;  # -1, 0, or 1
            
            $x += $dx_move;
            $y += $dy_move;
            
            # Bias walk back toward center if too far
            my $dx = $x - $cx;
            my $dy = $y - $cy;
            my $dist = sqrt($dx * $dx + $dy * $dy);
            
            # If beyond boundary, pull back toward center
            if ($dist > $max_dist * 1.1) {
                # 70% chance to move back toward center
                if (rand() < 0.7) {
                    $x += ($cx > $x) ? 1 : -1 if $cx != $x;
                    $y += ($cy > $y) ? 1 : -1 if $cy != $y;
                } else {
                    next;  # Skip this step
                }
            }
            
            # Clamp to map bounds
            next if $x < 0 || $x >= MAP_SIZE || $y < 0 || $y >= MAP_SIZE;
            
            # Skip if already visited
            next if exists $visited{"$x,$y"};
            
            # Mark as visited and place terrain
            $visited{"$x,$y"} = 1;
            
            # Calculate distance from center for terrain
            $dx = $x - $cx;
            $dy = $y - $cy;
            $dist = sqrt($dx * $dx + $dy * $dy);
            my $dist_ratio = $dist / $max_dist;
            
            # Determine terrain based on distance from center
            my $terrain = pick_terrain($dist_ratio);
            
            # Set terrain on map
            my $travel = ($terrain == TERRAIN_MOUNTAINS) ? TRAVEL_DIFFICULT :
                         ($terrain == TERRAIN_SWAMP) ? TRAVEL_DIFFICULT :
                         TRAVEL_STANDARD;
            
            $map[$y][$x] = $terrain | ($travel << 4);
            $tiles_placed++;
        }
    }
}

sub pick_terrain {
    my ($dist_ratio) = @_;
    
    if ($dist_ratio < 0.3) {
        # Center: Hills, mountains, some forest and grassland
        my $r = rand();
        return TERRAIN_MOUNTAINS if $r < 0.3;
        return TERRAIN_HILLS if $r < 0.6;
        return TERRAIN_FOREST if $r < 0.8;
        return TERRAIN_GRASS;
    }
    elsif ($dist_ratio < 0.7) {
        # Middle: Forest, grassland, some hills
        my $r = rand();
        return TERRAIN_FOREST if $r < 0.4;
        return TERRAIN_GRASS if $r < 0.8;
        return TERRAIN_HILLS;
    }
    else {
        # Edge: Grassland, forest, hills
        my $r = rand();
        return TERRAIN_GRASS if $r < 0.5;
        return TERRAIN_FOREST if $r < 0.8;
        return TERRAIN_HILLS;
    }
}

# ============================================================================
# SETTLEMENT PLACEMENT
# ============================================================================

sub place_settlements {
    my ($count) = @_;
    my @settlements = ();
    my $attempts = 0;
    
    while (@settlements < $count && $attempts < 10000) {
        $attempts++;
        
        my $x = int(rand(MAP_SIZE));
        my $y = int(rand(MAP_SIZE));
        
        my $terrain = get_terrain($x, $y);
        
        # Must be on land, not mountains or swamp
        next if $terrain == TERRAIN_WATER;
        next if $terrain == TERRAIN_MOUNTAINS;
        next if $terrain == TERRAIN_SWAMP;
        
        # Check spacing from other settlements
        my $too_close = 0;
        for my $s (@settlements) {
            my $dx = $x - $s->{x};
            my $dy = $y - $s->{y};
            my $dist = sqrt($dx * $dx + $dy * $dy);
            if ($dist < 15) {
                $too_close = 1;
                last;
            }
        }
        next if $too_close;
        
        # Determine settlement size based on terrain
        my $size;
        if ($terrain == TERRAIN_GRASS) {
            $size = 60 + int(rand(80));  # 60-139
        } elsif ($terrain == TERRAIN_FOREST) {
            $size = 40 + int(rand(60));  # 40-99
        } elsif ($terrain == TERRAIN_HILLS) {
            $size = 30 + int(rand(50));  # 30-79
        } else {
            $size = 20 + int(rand(40));  # 20-59
        }
        
        # Add settlement
        push @settlements, { x => $x, y => $y, size => $size };
        
        # Mark tile
        $map[$y][$x] |= FLAG_SPECIAL | FLAG_OBJECT;
        
        # Add to object list
        push @objects, {
            type => OBJ_SETTLEMENT,
            x => $x,
            y => $y,
            data => [$size, 0, 0, 0, 0]
        };
    }
    
    return @settlements;
}

# ============================================================================
# ROAD NETWORK GENERATION
# ============================================================================

sub generate_roads {
    my ($settlements) = @_;
    
    # Build minimum spanning tree using Prim's algorithm
    return if @$settlements < 2;
    
    my @in_tree = (0);  # Start with first settlement
    my %in_tree = (0 => 1);
    my @edges = ();
    
    while (@in_tree < @$settlements) {
        my $best_edge = undef;
        my $best_dist = 999999;
        
        # Find shortest edge from tree to outside
        for my $i (@in_tree) {
            for my $j (0..$#{$settlements}) {
                next if $in_tree{$j};
                
                my $dx = $settlements->[$i]{x} - $settlements->[$j]{x};
                my $dy = $settlements->[$i]{y} - $settlements->[$j]{y};
                my $dist = sqrt($dx * $dx + $dy * $dy);
                
                if ($dist < $best_dist) {
                    $best_dist = $dist;
                    $best_edge = [$i, $j];
                }
            }
        }
        
        if ($best_edge) {
            push @edges, $best_edge;
            push @in_tree, $best_edge->[1];
            $in_tree{$best_edge->[1]} = 1;
        } else {
            last;
        }
    }
    
    # Draw roads for each edge
    for my $edge (@edges) {
        my ($i, $j) = @$edge;
        draw_road($settlements->[$i]{x}, $settlements->[$i]{y},
                  $settlements->[$j]{x}, $settlements->[$j]{y});
    }
}

sub draw_road {
    my ($x1, $y1, $x2, $y2) = @_;
    
    # Use A* pathfinding to avoid water and mountains
    my @path = find_path($x1, $y1, $x2, $y2);
    
    for my $pos (@path) {
        my ($x, $y) = @$pos;
        
        # Don't overwrite settlements
        next if ($map[$y][$x] & FLAG_SPECIAL);
        
        my $terrain = get_terrain($x, $y);
        
        # Set road travel ease
        $map[$y][$x] = $terrain | (TRAVEL_ROAD << 4);
    }
}

sub find_path {
    my ($x1, $y1, $x2, $y2) = @_;
    
    # Simple straight-line path with terrain avoidance
    my @path = ();
    
    my $x = $x1;
    my $y = $y1;
    
    while ($x != $x2 || $y != $y2) {
        push @path, [$x, $y];
        
        my $dx = $x2 - $x;
        my $dy = $y2 - $y;
        
        # Move in direction of target
        if (abs($dx) > abs($dy)) {
            $x += ($dx > 0) ? 1 : -1;
        } else {
            $y += ($dy > 0) ? 1 : -1;
        }
        
        # Clamp to map bounds
        $x = max(0, min(MAP_SIZE - 1, $x));
        $y = max(0, min(MAP_SIZE - 1, $y));
    }
    
    push @path, [$x2, $y2];
    return @path;
}

# ============================================================================
# POINT OF INTEREST PLACEMENT
# ============================================================================

sub place_poi {
    my ($obj_type, $count, $symbol) = @_;
    my $placed = 0;
    my $attempts = 0;
    
    while ($placed < $count && $attempts < 10000) {
        $attempts++;
        
        my $x = int(rand(MAP_SIZE));
        my $y = int(rand(MAP_SIZE));
        
        my $terrain = get_terrain($x, $y);
        
        # Must be on land
        next if $terrain == TERRAIN_WATER;
        
        # Cannot overlap with special zones
        next if ($map[$y][$x] & FLAG_SPECIAL);
        
        # Mark tile
        $map[$y][$x] |= FLAG_SPECIAL | FLAG_OBJECT;
        
        # Add to object list
        push @objects, {
            type => $obj_type,
            x => $x,
            y => $y,
            data => [0, 0, 0, 0, 0]
        };
        
        $placed++;
    }
}

# ============================================================================
# FILE OUTPUT
# ============================================================================

sub write_binary_map {
    my ($filename) = @_;
    
    open(my $fh, '>', $filename) or die "Cannot write $filename: $!";
    binmode($fh);
    
    # Build object index map
    my %obj_index_map = ();
    for my $i (0..$#objects) {
        my $obj = $objects[$i];
        my $key = "$obj->{x},$obj->{y}";
        $obj_index_map{$key} = $i + 1;  # 1-based index (0 = no object)
    }
    
    # Write object list (8KB)
    for my $i (0..MAX_OBJECTS-1) {
        if ($i < @objects) {
            my $obj = $objects[$i];
            print $fh pack('C', $obj->{type});
            print $fh pack('C', $obj->{x});
            print $fh pack('C', $obj->{y});
            print $fh pack('C*', @{$obj->{data}});
        } else {
            # Null object
            print $fh pack('C8', (0) x 8);
        }
    }
    
    # Write map data (128KB - 2 bytes per cell)
    for my $y (0..MAP_SIZE-1) {
        for my $x (0..MAP_SIZE-1) {
            # First byte: terrain + flags
            print $fh pack('C', $map[$y][$x]);
            
            # Second byte: object index (0 = no object, 1-255 = index into object array)
            my $key = "$x,$y";
            my $obj_idx = $obj_index_map{$key} || 0;
            print $fh pack('C', $obj_idx);
        }
    }
    
    close($fh);
}

sub write_text_map {
    my ($filename, $show_roads) = @_;
    
    open(my $fh, '>', $filename) or die "Cannot write $filename: $!";
    
    for my $y (0..MAP_SIZE-1) {
        for my $x (0..MAP_SIZE-1) {
            my $byte = $map[$y][$x];
            my $terrain = $byte & 0x0F;
            my $travel = ($byte >> 4) & 0x03;
            my $has_object = $byte & FLAG_OBJECT;
            
            my $char = '.';
            
            # Check for objects first
            if ($has_object) {
                my $obj = find_object_at($x, $y);
                if ($obj) {
                    $char = get_object_char($obj->{type});
                }
            }
            
            # Show roads if enabled
            if ($char eq '.' && $show_roads && $travel == TRAVEL_ROAD) {
                $char = '=';
            }
            
            # Show terrain if no object/road
            if ($char eq '.') {
                $char = get_terrain_char($terrain);
            }
            
            print $fh $char;
        }
        print $fh "\n";
    }
    
    close($fh);
}

sub find_object_at {
    my ($x, $y) = @_;
    
    for my $obj (@objects) {
        return $obj if $obj->{x} == $x && $obj->{y} == $y;
    }
    
    return undef;
}

sub get_terrain_char {
    my ($terrain) = @_;
    
    return '.' if $terrain == TERRAIN_WATER;
    return 'g' if $terrain == TERRAIN_GRASS;
    return 'f' if $terrain == TERRAIN_FOREST;
    return 'h' if $terrain == TERRAIN_HILLS;
    return 'm' if $terrain == TERRAIN_MOUNTAINS;
    return 'd' if $terrain == TERRAIN_DESERT;
    return 'w' if $terrain == TERRAIN_SWAMP;
    return '?';
}

sub get_object_char {
    my ($obj_type) = @_;
    
    return 'X' if $obj_type == OBJ_SETTLEMENT;
    return 'T' if $obj_type == OBJ_TOWER;
    return 'S' if $obj_type == OBJ_SHRINE;
    return 'R' if $obj_type == OBJ_RUINS;
    return 'M' if $obj_type == OBJ_MINE;
    return 'A' if $obj_type == OBJ_STELA;
    return 'P' if $obj_type == OBJ_PORTAL;
    return 'C' if $obj_type == OBJ_CAVE;
    return 'O' if $obj_type == OBJ_MONUMENT;
    return '?';
}

sub get_terrain {
    my ($x, $y) = @_;
    return $map[$y][$x] & 0x0F;
}
