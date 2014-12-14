#											power_supply_logger_parser.pl Intial Version
# This parser takes a bin file and give the user the option to select either text format output or csv file output.
#The output file name will be power_supply_log_<todays date & time >.txt/csv.

# Below is the structure of the Logger
#typedef struct {
#    char ac_online;
#    char ac_present;
#} __attribute__ ((packed)) ac_info_t;

#typedef struct {
#    char capacity;
#   char power_supply_present;
#    int charge_counter;
#    int energy_full;
#    int cycle_count;
#    int temp;
#    int voltage_now;
#    int current_now;
#    int voltage_max_design;
#    int voltage_min_design;
#    char power_supply_status; //char to represent strings Charging,Discharging etc
#    char power_supply_health; //char to represent strings Good,Bad etc
#    char power_supply_charge_type; //char to represent strings Fast,Unknown etc
#}__attribute__ ((packed)) battery_info_t;

#typedef struct {
#    char usb_online;
#     char usb_present;
#}__attribute__ ((packed)) usb_info_t;

#typedef struct {
#    struct timeval timestamp;
#    long int uptime;
#    ac_info_t ac_info;
#    battery_info_t battery_info;
#    usb_info_t usb_info;
#}__attribute__ ((packed)) power_supply_t;


# #Structure of the New Logger Version

# /* The logger structure  */
# typedef struct {
    # char type;
    # struct timeval timestamp;
    # long int uptime;
# }__attribute__ ((packed)) logger_record_hdr_t; // 13 bytes in total

# typedef struct {
    # char ac_online;
    # char ac_present;
# } __attribute__ ((packed)) ac_info_t;

# typedef struct {
    # char capacity;
    # char power_supply_present;
    # int charge_counter;
    # int energy_full;
    # int cycle_count;
    # int temp;
    # int voltage_now;
    # int current_now;
    # char power_supply_status; //char to represent strings Charging,Discharging etc
    # char power_supply_health; //char to represent strings Good,Bad etc
    # char power_supply_charge_type; //char to represent strings Fast,Unknown etc
# }__attribute__ ((packed)) battery_info_t;

# typedef struct {
    # char usb_online;
    # char usb_present;
# }__attribute__ ((packed)) usb_info_t;

# typedef struct {
    # char extdock_state;
    # char semu_audio_state;
    # char smartdock_state;
# }__attribute__ ((packed)) acc_status_t;

# typedef struct {
    # logger_record_hdr_t hdr;
    # ac_info_t ac_info;
    # battery_info_t battery_info;
    # usb_info_t usb_info;
    # acc_status_t acc_info;
    # char fsm_state;
    # char max_charge_rate;
    # int last_ocv_uv;
# }__attribute__ ((packed)) power_supply_t;// 55 bytes in total

#define PROPERTY_VALUE_MAX 92
# typedef struct
# {
    # logger_record_hdr_t hdr;
    # char phone_flash_version[PROPERTY_VALUE_MAX];
    # char phone_bootmode[BOOT_MODE_LENGTH];
    # char battery_valid;
    # int voltage_max_design;
    # int voltage_min_design;
# } __attribute__ ((packed))  power_up_t; // 144 Bytes in Total


#Getting the input .bin file to be parsed.
$numArgs = $#ARGV + 1;
if ($numArgs < 1)
{
	print "You need to provide the file name";
	exit;
}
print "Trying to open file $ARGV[$argnum]\n";

open my $in_fh, $ARGV[$argnum] or die $!;
binmode $in_fh;
use POSIX;
#creating the file which would contain the parsed output
$logfilename = "power_supply_log";
$displayed = 0;
$power_up_record_size = 144;
$power_supply_record_size = 55;
my ($sec, $min, $hour, $day, $month, $year, $wday, $yday, $dst) = localtime();
$logfilename = sprintf ("%s_%d-%d-%d_time_%d_%d",$logfilename,$day, $month+1, ($year + 1900),$hour, $min);
printf ("Which type of file to be created? \nEnter 1 for .txt file any other key for csv - comma separarated values file\n");
$filetype = <STDIN>;
$type = 0;
if ($filetype == 1)
{
	print "Generating .txt file\n";
	$logfilename = sprintf ("%s.txt",$logfilename);
}
else
{
        $type = 2;
	print "Generating .csv file\n";
	$logfilename = sprintf ("%s.csv",$logfilename);
	print LOG_FILE "HEADER ","\t","NEXT";
}
printf ("file name is %s\n", $logfilename);
open (LOG_FILE, ">", $logfilename);

my ($buf, $data, $n);

#Get the file size, calculate how many chunks are there...
my ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks) = stat($in_fh);

if (read($in_fh, $data, 1) != 1)
{
	print "Error reading file!\n";
	die;
}
$data = unpack("C",$data);

if ($data == 0x01 ) {
	while(!eof($in_fh) > 0) {
		if (read($in_fh, $data, 1) != 1) {
			print "Error reading file!\n";
			die;
		}
		$data = unpack("C",$data);

		if ($data == 1) {
			$rec_ref = read_record_power_up($in_fh);
			%rec = %{$rec_ref};
			print LOG_FILE "\n";
			print LOG_FILE "Phone powered on Date = ", $rec{'fmt_timestamp'},"\n";
			print LOG_FILE "Uptime = ",$rec{'uptime'}," seconds","\n";
			print LOG_FILE "Phone flash version = ",$rec{'phone_flash_version'},"\n";
			print LOG_FILE "Phone boot mode  = ",$rec{'phone_boot_mode'},"\n";
			print LOG_FILE "battery valid  = ",$rec{'battey_valid'},"\n";
			print LOG_FILE "voltage max design  = ",$rec{'voltage_max_design'}," microvolts","\n";
			print LOG_FILE "voltage min design   = ",$rec{'voltage_min_design'}," microvolts","\n";
			print LOG_FILE "\n ";
			$displayed = 0;
		}
		elsif ($data == 2) {
			$rec_ref = read_record_power_supply($in_fh);
			%rec = %{$rec_ref};
			if ($type == 2) {
				# This is done to write the header info to the csv file after a powerup log is read.
				if ($displayed == 0) {
					print LOG_FILE "timestamp,uptime in seconds,capacity,ps_present,charger_counter,energy_full,cycle_count,temp,voltage_now in microvolts,current_now in microamps,ps_status,ps_health,charge_type,ac_online,ac_present,usb_online,usb_present,extdock state,semu audio state,smart dock state,fsm state,max charge rate,last ocv uv\n";
					$displayed = 1;
				}
				print LOG_FILE "'$rec{'fmt_timestamp'},";
				print LOG_FILE "$rec{'uptime'},";
				print LOG_FILE "$rec{'capacity'},";
				print LOG_FILE "$rec{'fmt_ps_present'},";
				print LOG_FILE "$rec{'charge_counter'},";
				print LOG_FILE "$rec{'energy_full'},";
				print LOG_FILE "$rec{'cycle_count'},";
				print LOG_FILE "$rec{'temp'},";
				print LOG_FILE "$rec{'voltage_now'},";
				print LOG_FILE "$rec{'current_now'},";
				print LOG_FILE "$rec{'fmt_ps_status'},";
				print LOG_FILE "$rec{'fmt_ps_health'},";
				print LOG_FILE "$rec{'fmt_ps_charge_type'},";
				print LOG_FILE "$rec{'fmt_ac_online'},";
				print LOG_FILE "$rec{'fmt_ac_present'},";
				print LOG_FILE "$rec{'fmt_usb_online'},";
				print LOG_FILE "$rec{'fmt_usb_present'},";
				print LOG_FILE "$rec{'fmt_extdock_state'},";
				print LOG_FILE "$rec{'fmt_semu_audio_state'},";
				print LOG_FILE "$rec{'fmt_smartdock_state'},";
				print LOG_FILE "$rec{'fmt_fsm_state'},";
				print LOG_FILE "$rec{'fmt_max_charge_rate'},";
				print LOG_FILE "$rec{'last_ocv_uv'}";
			}
			else {
				print LOG_FILE "Date = ", $rec{'fmt_timestamp'},"\n";
				print LOG_FILE "Uptime = ",$rec{'uptime'}," seconds","\n";
				print LOG_FILE "Battery Capacity = ", $rec{'capacity'},"\n";
				print LOG_FILE "Power Supply Present = ", $rec{'fmt_ps_present'},"\n";
				print LOG_FILE "Charge Counter = ",$rec{'charge_counter'},"\n";
				print LOG_FILE "Energy Full = ",$rec{'energy_full'},"\n";
				print LOG_FILE "Cycle Count = ",$rec{'cycle_count'},"\n";
				print LOG_FILE "Temperature = ", $rec{'temp'},"\n";
				print LOG_FILE "Voltage Now = ", $rec{'voltage_now'},"\n";
				print LOG_FILE "Current Now = ", $rec{'current_now'},"\n";
				print LOG_FILE "Power Supply Status = ", $rec{'fmt_ps_status'},"\n";
				print LOG_FILE "Power Supply Health = ", $rec{'fmt_ps_health'},"\n";
				print LOG_FILE "Charge Type = ", $rec{'fmt_ps_charge_type'},"\n";
				print LOG_FILE "AC online = ",$rec{'fmt_ac_online'},"\n";
				print LOG_FILE "AC present = ",$rec{'fmt_ac_present'},"\n";
				print LOG_FILE "USB online = ",$rec{'fmt_usb_online'},"\n";
				print LOG_FILE "USB present = ",$rec{'fmt_usb_present'},"\n";
				print LOG_FILE "Ext Dock State = ",$rec{'fmt_extdock_state'},"\n";
				print LOG_FILE "Semu Audio State = ",$rec{'fmt_semu_audio_state'},"\n";
				print LOG_FILE "Smart Dock State = ",$rec{'fmt_smartdock_state'},"\n";
				print LOG_FILE "FSM State = ",$rec{'fmt_fsm_state'},"\n";
				print LOG_FILE "Max Charge Rate = ",$rec{'fmt_max_charge_rate'},"\n";
				print LOG_FILE "Last ocv uv  = ",$rec{'last_ocv_uv'},"\n";
			}
			print LOG_FILE "\n";
		}
		else {
			print "Unknown file Type %d ", $data,"\n";
			die;
		}
	}  # end of WHILE Loop
}  # Version 1 if cond ends here

# The initial Version formatting which didnt have a header starts here.
else {
	initial_version_logger_v0($in_fh);
}
close (LOG_FILE);
close($in_fh);

# The initial version formatting of logs
sub initial_version_logger_v0
{
#if user selects csv format, update the file with headers...
	if ($type == 2) {
		print LOG_FILE "timestamp,uptime in seconds,capacity,ps_present,charger_counter,energy_full,cycle_count,temp,voltage_now in microvolts,current_now in microamps,voltage_max_design in microvolts,voltage_min_design in microvolts,ps_status,ps_health,charge_type,ac_online,ac_present,usb_online,usb_present\n";
	}
	seek($in_fh, 0, 0);

	#Parsing the logged data in a for loop
	while(!eof($in_fh) > 0) {
		$rec_ref = read_record_v0($in_fh);
		%rec = %{$rec_ref};

		if ($type == 2) {
			print LOG_FILE "'$rec{'fmt_timestamp'},";
			print LOG_FILE "$rec{'uptime'},";
			print LOG_FILE "$rec{'capacity'},";
			print LOG_FILE "$rec{'fmt_ps_present'},";
			print LOG_FILE "$rec{'charge_counter'},";
			print LOG_FILE "$rec{'energy_full'},";
			print LOG_FILE "$rec{'cycle_count'},";
			print LOG_FILE "$rec{'temp'},";
			print LOG_FILE "$rec{'voltage_now'},";
			print LOG_FILE "$rec{'current_now'},";
			print LOG_FILE "$rec{'voltage_max_design'},";
			print LOG_FILE "$rec{'voltage_min_design'},";
			print LOG_FILE "$rec{'fmt_ps_status'},";
			print LOG_FILE "$rec{'fmt_ps_health'},";
			print LOG_FILE "$rec{'fmt_ps_charge_type'},";
			print LOG_FILE "$rec{'fmt_ac_online'},";
			print LOG_FILE "$rec{'fmt_ac_present'},";
			print LOG_FILE "$rec{'fmt_usb_online'},";
			print LOG_FILE "$rec{'fmt_usb_present'}";
		}
		else {
			print LOG_FILE "Date = ", $rec{'fmt_timestamp'},"\n";
			print LOG_FILE "Uptime = ",$rec{'uptime'}," seconds","\n";
			print LOG_FILE "Battery Capacity = ", $rec{'capacity'},"\n";
			print LOG_FILE "Power Supply Present = ", $rec{'fmt_ps_present'},"\n";
			print LOG_FILE "Charge Counter = ",$rec{'charge_counter'},"\n";
			print LOG_FILE "Energy Full = ",$rec{'energy_full'},"\n";
			print LOG_FILE "Cycle Count = ",$rec{'cycle_count'},"\n";
			print LOG_FILE "Temperature = ", $rec{'temp'},"\n";
			print LOG_FILE "Voltage Now = ", $rec{'voltage_now'},"\n";
			print LOG_FILE "Current Now = ", $rec{'current_now'},"\n";
			print LOG_FILE "Voltage Max Design = ", $rec{'voltage_max_design'},"\n";
			print LOG_FILE "Voltage Min Design = ", $rec{'voltage_min_design'},"\n";
			print LOG_FILE "Power Supply Status = ", $rec{'fmt_ps_status'},"\n";
			print LOG_FILE "Power Supply Health = ", $rec{'fmt_ps_health'},"\n";
			print LOG_FILE "Charge Type = ", $rec{'fmt_ps_charge_type'},"\n";
			print LOG_FILE "AC online = ",$rec{'fmt_ac_online'},"\n";
			print LOG_FILE "AC present = ",$rec{'fmt_ac_present'},"\n";
			print LOG_FILE "USB online = ",$rec{'fmt_usb_online'},"\n";
			print LOG_FILE "USB present = ",$rec{'fmt_usb_present'},"\n";
		}
		print LOG_FILE "\n";

	}
}

sub read_record_power_up
{
	my $fh = shift;
	my %rec = ();

	# power_up_record_size - 1 since 1 byte record_type is already read.
	if (read($fh, $data, $power_up_record_size - 1) != $power_up_record_size - 1) {
		print "Error reading file!\n";
		die;
	}
	else {
		($rec{'timestamp_sec'}, $rec{'timestamp_usec'}, $rec{'uptime'},
		$rec{'phone_flash_version'},
		$rec{'phone_boot_mode'},
		$rec{'battey_valid'},
		$rec{'voltage_max_design'},
		$rec{'voltage_min_design'}
		) =
		unpack("V3a92a30C1iV2", $data);

		$rec{'fmt_timestamp'} = sprintf("%s.%06d",
					 strftime("%Y/%m/%d %H:%M:%S", localtime($rec{'timestamp_sec'})),
					 $rec{'timestamp_usec'});
		$displayed == 0;
	}
	return \%rec;
}


sub read_record_power_supply
{
	my $fh = shift;
	my %rec = ();
	# power_supply_record_size - 1 since 1 byte record_type is already read.
	if (read($fh, $data, $power_supply_record_size - 1) != $power_supply_record_size - 1)
	{
		print "Error reading file!\n";
		die;
	}
	else {
		($rec{'timestamp_sec'}, $rec{'timestamp_usec'}, $rec{'uptime'},
		$rec{'ac_online'}, $rec{'ac_present'},$rec{'capacity'}, $rec{'ps_present'},
		$rec{'charge_counter'}, $rec{'energy_full'},$rec{'cycle_count'}, $rec{'temp'}, $rec{'voltage_now'}, $rec{'current_now'},
		$rec{'ps_status'}, $rec{'ps_health'}, $rec{'ps_charge_type'}, $rec{'usb_online'}, $rec{'usb_present'},
		$rec{' extdock_state'},$rec{'semu_audio_state'},$rec{'smartdock_state'},$rec{'fsm_state'},$rec{'max_charge_rate'},
		$rec {'last_ocv_uv'}) =

		unpack("V3C4iV4iC6C4V", $data);
		$rec{'fmt_timestamp'} = sprintf("%s.%06d",
					 strftime("%Y/%m/%d %H:%M:%S", localtime($rec{'timestamp_sec'})),
					 $rec{'timestamp_usec'});
		$rec{'fmt_ac_online'} = $rec{'ac_online'} == 1 ? "True" : "False";
		$rec{'fmt_ac_present'} = $rec{'ac_present'} == 1 ? "True" : "False";
		$rec{'fmt_usb_online'} = $rec{'usb_online'} == 1 ? "True" : "False";
		$rec{'fmt_usb_present'} = $rec{'usb_present'} == 1 ? "True" : "False";
		$rec{'fmt_ps_present'} = $rec{'ps_present'} == 1 ? "True" : "False";
		$rec{'fmt_ps_health'} = $rec{'ps_health'} == 1 ? "Good" : "Unknown";
		$rec{'fmt_ps_charge_type'} = $rec{'ps_charge_type'} == 1 ? "Fast" : "n/a";

		if ($rec{'ps_status'} == "1") {
			$rec{'fmt_ps_status'} = "Not Charging";
		} elsif ($rec{'ps_status'} == 2) {
			$rec{'fmt_ps_status'} = "Charging";
		} elsif ($rec{'ps_status'} == 3) {
			$rec{'fmt_ps_status'} = "Full";
		} elsif ($rec{'ps_status'} == 4) {
			$rec{'fmt_ps_status'} = "Discharging";
		} else {
			$rec{'fmt_ps_status'} = "Unknown";
		}

		if ($rec{'extdock_state'} == "1") {
			$rec{'fmt_extdock_state'} = "Desk Dock";
		} elsif ($rec{'extdock_state'} == 2) {
			$rec{'fmt_extdock_state'} = "Car Dock";
		} elsif ($rec{'extdock_state'} == 3) {
			$rec{'fmt_extdock_state'} = "LE Dock";
		} elsif ($rec{'extdock_state'} == 4) {
			$rec{'fmt_extdock_state'} = "HE Dock";
		} elsif ($rec{'extdock_state'} == 5) {
			$rec{'fmt_extdock_state'} = "Moblie Dock";
		} else {
			$rec{'fmt_extdock_state'} = "No Dock";
		}

		$rec{'fmt_semu_audio_state'} = $rec{'semu_audio_state'} == 1 ? "Audio cable present" : "None";
		$rec{'fmt_smartdock_state'} = $rec{'smartdock_state'} == 1 ? "Docked" : "Undocked";

		if ($rec{'fsm_state'} == "0") {
			$rec{'fmt_fsm_state'} = "FSM_STATE_OFF";
		} elsif ($rec{'fsm_state'} == 1) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ON_CHG_HIGH";
		} elsif ($rec{'fsm_state'} == 2) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ATC_2A";
		} elsif ($rec{'fsm_state'} == 3) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ON_BAT_3";
		} elsif ($rec{'fsm_state'} == 4) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ATC_FAIL_4";
		} elsif ($rec{'fsm_state'} == 5) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_DELAY_5";
		} elsif ($rec{'fsm_state'} == 6) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ON_CHG_AND_BAT_6";
		} elsif ($rec{'fsm_state'} == 7) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_FAST_CHG_7";
		} elsif ($rec{'fsm_state'} == 8) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_TRKL_CHG_8";
		} elsif ($rec{'fsm_state'} == 9) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_CHG_FAIL_9";
		} elsif ($rec{'fsm_state'} == 10) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_EOC_10";
		} elsif ($rec{'fsm_state'} == 11) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ON_CHG_VREGOK_11";
		} elsif ($rec{'fsm_state'} == 12) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_BATFETDET_START_12";
		} elsif ($rec{'fsm_state'} == 13) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ATC_PAUSE_13";
		} elsif ($rec{'fsm_state'} == 14) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_FAST_CHG_PAUSE_14";
		} elsif ($rec{'fsm_state'} == 15) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_TRKL_CHG_PAUSE_15";
		} elsif ($rec{'fsm_state'} == 16) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_BATFETDET_END_16 = 16";
		} elsif ($rec{'fsm_state'} == 18 ) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_ATC_2B";
		} elsif ($rec{'fsm_state'} == 20) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_START_BOOT";
		} elsif ($rec{'fsm_state'} == 21) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_FLCB_VREGOK";
		} elsif ($rec{'fsm_state'} == 22) {
			$rec{'fmt_fsm_state'} = "FSM_STATE_FLCB";
		} else {
			$rec{'fmt_fsm_state'} = "Unknown";
		}

		if ($rec{'max_charge_rate'} == 0) {
			$rec{'fmt_max_charge_rate'} = 100;
		} elsif ($rec{'max_charge_rate'} == 1) {
			$rec{'fmt_max_charge_rate'} = 500;
		} elsif ($rec{'max_charge_rate'} == 2) {
			$rec{'fmt_max_charge_rate'} = 700;
		} elsif ($rec{'max_charge_rate'} == 3) {
			$rec{'fmt_max_charge_rate'} = 850;
		} elsif ($rec{'max_charge_rate'} == 4) {
			$rec{'fmt_max_charge_rate'} = 900;
		} elsif ($rec{'max_charge_rate'} == 5) {
			$rec{'fmt_max_charge_rate'} = 1100;
		} elsif ($rec{'max_charge_rate'} == 6) {
			$rec{'fmt_max_charge_rate'} = 1300;
		} elsif ($rec{'max_charge_rate'} == 7) {
			$rec{'fmt_max_charge_rate'} = 1500;
		} else {
			$rec{'fmt_max_charge_rate'} = "Unknown";
		}
	}

	return \%rec;
}


sub read_record_v0
{
	my $fh = shift;
	my %rec = ();

	if (read($fh, $data, 53) != 53)
	{
		print "Error reading file!\n";
	}
	else
	{

		($rec{'timestamp_sec'}, $rec{'timestamp_usec'}, $rec{'uptime'}, $rec{'ac_online'}, $rec{'ac_present'},
		  $rec{'capacity'}, $rec{'ps_present'}, $rec{'charge_counter'}, $rec{'energy_full'},
		  $rec{'cycle_count'}, $rec{'temp'}, $rec{'voltage_now'}, $rec{'current_now'},
		  $rec{'voltage_max_design'}, $rec{'voltage_min_design'}, $rec{'ps_status'},
		  $rec{'ps_health'}, $rec{'ps_charge_type'}, $rec{'usb_online'}, $rec{'usb_present'}) =
		    unpack("V3C4iV4iV2C5", $data);

		$rec{'fmt_timestamp'} = sprintf("%s.%06d",
					 strftime("%Y/%m/%d %H:%M:%S", localtime($rec{'timestamp_sec'})),
					 $rec{'timestamp_usec'});
		$rec{'fmt_ac_online'} = $rec{'ac_online'} == 1 ? "True" : "False";
		$rec{'fmt_ac_present'} = $rec{'ac_present'} == 1 ? "True" : "False";
		$rec{'fmt_usb_online'} = $rec{'usb_online'} == 1 ? "True" : "False";
		$rec{'fmt_usb_present'} = $rec{'usb_present'} == 1 ? "True" : "False";
		$rec{'fmt_ps_present'} = $rec{'ps_present'} == 1 ? "True" : "False";
		$rec{'fmt_ps_health'} = $rec{'ps_health'} == 1 ? "Good" : "Unknown";
		$rec{'fmt_ps_charge_type'} = $rec{'ps_charge_type'} == 1 ? "Fast" : "n/a";

		if ($rec{'ps_status'} == "1") {
			$rec{'fmt_ps_status'} = "Not Charging";
		} elsif ($rec{'ps_status'} == 2) {
			$rec{'fmt_ps_status'} = "Charging";
		} elsif ($rec{'ps_status'} == 3) {
			$rec{'fmt_ps_status'} = "Full";
		} elsif ($rec{'ps_status'} == 4) {
			$rec{'fmt_ps_status'} = "Discharging";
		} else {
			$rec{'fmt_ps_status'} = "Unknown";
		}
	}

	return \%rec;
}

