import struct
import os
import fnmatch
import array
import time
import sys
from operator import itemgetter, attrgetter

fields_hdr = ('hdr_text',
              'hdr_version',
              'hdr_file_write_count',
              'hdr_time_stamp')
fields_data_v0 = ('real_fcc_batt_temp',
                  'real_fcc',
                  'soc',
                  'ocv_uv',
                  'rbatt',
                  'charge_increase',
                  'chargecycles',
                  'aged_capacity',
                  'boc_percent',
                  'boc_ocv_uv',
                  'boc_cc_mah',
                  'eoc_percent',
                  'eoc_ocv_uv',
                  'eoc_cc_mah',
                  'aged_timestamp')

def file_read(filename):
    print "- Reading file " + filename
    fh = open(filename, 'rb')
    is_success, hdr = file_read_header(fh)
    if is_success == True:
        if hdr['hdr_version'] == 1:
            is_success, data = file_read_data_v0(fh)
        elif hdr['hdr_version'] == 0:
            print "--- ERROR: Unsupported version: " + hdr['hdr_version']
        else:
            print "--- ERROR: Unknown version: " + hdr['hdr_version']
            is_success = False

    if is_success == True:
        file_data = dict(hdr.items() + data.items())
    else:
        file_data = dict()
    return is_success, file_data

def file_read_header(fh):
    is_success = False
    line = fh.read(20)
    hdr = dict(zip(fields_hdr, struct.unpack("<4siQi", line)))
    if hdr['hdr_text'] != 'BHD\0':
        print "--- ERROR - Corrupted header"
    else:
        is_success = True;
    return is_success, hdr

def file_read_data_v0(fh):
    is_success = False
    line = fh.read(60);
    data = dict(zip(fields_data_v0, struct.unpack("<15i", line)))
    is_success = True;
    return is_success, data


def main():
    try:
        data_list = list()

        if len(sys.argv) > 1:
            path = sys.argv[1] + '/'
        else:
            path = './'

        for file in os.listdir(path):
            if fnmatch.fnmatch(file, 'batt-????????????????_*'):
                is_success, data = file_read(path+file)
                if is_success == True:
                    data['uid'] = file[5:21]
                    data['filename'] = file
                    data_list.append(data)

        data_list = sorted(data_list, key=itemgetter('hdr_file_write_count'), reverse = True)
        data_list = sorted(data_list, key=itemgetter('uid'))

        last_battery = ""
        print_latest = 0;
        for data in data_list:
            if (last_battery != data['uid']):
                print "======================================================================================="
                print "Battery UID: " + data['uid']
                print "======================================================================================="
                print
                last_battery = data['uid']
                print_latest = 1

            if print_latest == 1:
                print "Filename: " + data['filename'] + " *LATEST*"
                print_latest = 0
            else:
                print "Filename: " + data['filename']
            print "====================================================\n"
            print "Header"
            print "------"
            print "\tFile Created:\t\t", time.asctime(time.gmtime(data['hdr_time_stamp'])),"(",data['hdr_time_stamp'],")"
            print "\tFile Write Count:\t", data['hdr_file_write_count']
            print "\tVersion:\t\t%d" % data['hdr_version']
            print "\nData"
            print "----"
            print "\tCharge Cycle Count:\t", data['chargecycles']
            print "\tCharge % Increase:\t", data['charge_increase']
            print "\tAged Capacity:\t\t", data['aged_capacity']
            print "\t\tAged Capacity Date:\t",
            if data['aged_timestamp'] == 0:
                print "Not Calculated"
            else:
                print time.asctime(time.gmtime(data['aged_timestamp'])),"(",data['aged_timestamp'],")"
            print "\t\tBOC - Percent:\t\t", data['boc_percent']
            print "\t\tBOC - OCV:\t\t", data['boc_ocv_uv']
            print "\t\tBOC - CC:\t\t", data['boc_cc_mah']
            print "\t\tEOC - Percent:\t\t", data['eoc_percent']
            print "\t\tEOC - OCV:\t\t", data['eoc_ocv_uv']
            print "\t\tEOC - CC:\t\t", data['eoc_cc_mah']
            print "\tReal FCC Batt Temp:\t", data['real_fcc_batt_temp']
            print "\tReal FCC:\t\t", data['real_fcc']
            print "\tState of Charge:\t", data['soc']
            print "\tOCV\t\t\t", data['ocv_uv']
            print "\tRBatt:\t\t\t", data['rbatt']
            print "\n"

        print "Done!"

    except Exception, err:
        print 'ERROR: ' + str(err)
        traceback.print_exc()


if __name__ == '__main__':
    main()
