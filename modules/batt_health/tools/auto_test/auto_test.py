import os
import struct
import time
import random
import traceback
import re

state = dict()
nvm_state = dict()

def adb_command(sub_cmd):
    rsp =''
    cmd = 'adb ' + sub_cmd
    res = os.popen(cmd, "r")
    while 1:
        line = res.readline()
        if not line: break
        rsp += line
    return rsp

def adb_wait_for_device():
    print "--- Waiting for device"
    adb_command('wait-for-device')

def adb_reboot():
    print "--- Rebooting phone"
    adb_command('reboot')
    adb_wait_for_device()

def adb_clear_bhd_data():
    print "--- Clearing battery health PDS data"
    adb_command('shell rm /pds/batt_health/*')

def adb_pull_bhd_data(local_path):
    print "--- Pulling battery health PDS data"
    adb_command('pull /pds/batt_health ' + local_path)

def adb_stop_bhd():
    adb_command('shell stop batt_health')

def adb_start_bhd():
    adb_command('shell start batt_health')

def adb_update_charge_state():
    global state

    string = struct.pack('<7i', state['ph_is_charging'], state['ph_soc'],
                  state['ph_cc_uah'], state['ph_real_fcc_batt_temp'],
                  state['ph_real_fcc'], state['ph_ocv'],
                  state['ph_rbatt'])
    string = '\\x' + '\\x'.join('%02x' % ord(b) for b in string)
    string = 'shell \"echo -e \'' + string + '\' > /sys/devices/platform/msm_ssbi.0/pm8921-core/pm8921-bms/override.bin\"'
    rsp = adb_command(string)
    if rsp != '':
        raise RuntimeError('Invalid response from phone = ' + rsp)

def state_init():
    global state

    state['ph_is_charging'] = 0
    state['ph_soc'] = 100
    state['ph_cc_uah'] = -22
    state['ph_real_fcc_batt_temp'] = -22
    state['ph_real_fcc'] = -22
    state['ph_ocv'] = -22
    state['ph_rbatt'] = -22
    state['charge_cycles'] = 0
    state['charge_inc'] = 0
    state['file_write_count'] = 0
    state['aged_begin_cc_uah']  = -22
    state['aged_begin_ocv']     = -22
    state['aged_begin_percent'] = -22
    state['aged_end_cc_uah']    = -22
    state['aged_end_ocv']       = -22
    state['aged_end_percent']   = -22


def nvm_state_copy():
    global state
    global nvm_state

    nvm_state['charge_cycles'] = state['charge_cycles']
    nvm_state['charge_inc'] = state['charge_inc']
    nvm_state['file_write_count'] = state['file_write_count']

    if (state['ph_real_fcc_batt_temp'] != -22):
        nvm_state['ph_real_fcc_batt_temp'] = state['ph_real_fcc_batt_temp']

    if (state['ph_real_fcc'] != -22):
        nvm_state['ph_real_fcc'] = state['ph_real_fcc']

    if (state['ph_soc'] != -22):
        nvm_state['ph_soc'] = state['ph_soc']

    if (state['ph_ocv'] != -22):
        nvm_state['ph_ocv'] = state['ph_ocv']

    if (state['ph_rbatt'] != -22):
        nvm_state['ph_rbatt'] = state['ph_rbatt']

    if (state['aged_begin_cc_uah'] != -22):
        nvm_state['aged_begin_cc_uah'] = state['aged_begin_cc_uah']

    if (state['aged_begin_percent'] != -22):
        nvm_state['aged_begin_percent'] = state['aged_begin_percent']

    if (state['aged_begin_ocv'] != -22):
        nvm_state['aged_begin_ocv'] = state['aged_begin_ocv']

    if (state['aged_begin_cc_uah'] != -22):
        nvm_state['aged_begin_cc_uah'] = state['aged_begin_cc_uah']

    if (state['aged_end_percent'] != -22):
        nvm_state['aged_end_percent'] = state['aged_end_percent']

    if (state['aged_end_ocv'] != -22):
        nvm_state['aged_end_ocv'] = state['aged_end_ocv']

    if (state['aged_end_cc_uah'] != -22):
        nvm_state['aged_end_cc_uah'] = state['aged_end_cc_uah']


def update_nvm_state():
    global state
    global nvm_state

    if (nvm_state['charge_cycles'] != state['charge_cycles']):
        print "--- NVM state updated due to charge cycles"
        state['file_write_count'] = state['file_write_count'] + 1
        nvm_state_copy()
    elif ( (state['charge_inc'] - nvm_state['charge_inc']) >= 50):
        print "--- NVM state updated due to charge increase"
        state['file_write_count'] = state['file_write_count'] + 1
        nvm_state_copy()

def execute_batt_health_reset():
    adb_stop_bhd()
    adb_clear_bhd_data()
    adb_reboot()
    adb_stop_bhd()
    adb_clear_bhd_data()
    adb_update_charge_state()
    adb_start_bhd()

def execute_discharge_cycle(target_soc, interval, sleep):
    global state

    print "--- Discharging from",state['ph_soc'],"to",target_soc
    state['ph_is_charging'] = 0
    adb_update_charge_state()
    time.sleep(sleep)

    while state['ph_soc'] > target_soc:
        state['ph_soc'] -= interval
        if (state['ph_soc'] < target_soc): state['ph_soc']  = target_soc
        adb_update_charge_state()
        time.sleep(sleep)

def execute_charge_cycle(target_soc, interval, sleep):
    global state

    print "--- Charging from",state['ph_soc'],"to",target_soc
    start_soc = state['ph_soc']
    state['ph_is_charging'] = 1
    if (0 <= state['ph_soc'] <= 5):
            state['ph_cc_uah'] = random.randint(1500000, 1766000)
            state['ph_ocv'] = random.randint(3200000, 4300000)
            begin_cc_uah = state['ph_cc_uah']
            begin_ocv = state['ph_ocv']

    adb_update_charge_state()
    time.sleep(sleep)

    while state['ph_soc'] < target_soc:
        state['ph_soc'] += interval
        if (state['ph_soc'] > target_soc): state['ph_soc']  = target_soc
        if ((95 <= state['ph_soc'] <= 100) and (state['ph_soc'] == target_soc)):
            state['ph_cc_uah'] = random.randint(0, 10000)
            state['ph_ocv'] = random.randint(3200000, 4300000)
            end_cc_uah = state['ph_cc_uah']
            end_ocv = state['ph_ocv']
        adb_update_charge_state()
        time.sleep(sleep)

    state['ph_is_charging'] = 0
    adb_update_charge_state()

    state['charge_inc'] += (target_soc - start_soc)
    if (state['charge_inc'] > 100):
        state['charge_cycles'] = state['charge_cycles'] + 1
        state['charge_inc'] = state['charge_inc'] % 100

    if ( (0 <= start_soc <= 5) and (95 <= target_soc <= 100)):
        print "--- Aged event occurred!"
        state['aged_begin_cc_uah']  = begin_cc_uah
        state['aged_begin_ocv']     = begin_ocv
        state['aged_begin_percent'] = start_soc
        state['aged_end_cc_uah']    = end_cc_uah
        state['aged_end_ocv']       = end_ocv
        state['aged_end_percent']   = target_soc

    update_nvm_state()

def test_case_random_event():
    global state
    global nvm_state

    i = random.randint(0, 100)

    if (i == 0):
        temp_soc = state['ph_soc']
        state = nvm_state.copy()
        state['ph_is_charging'] = 0
        state['ph_soc'] = temp_soc
        state['ph_cc_uah'] = -22
        state['ph_real_fcc_batt_temp'] = -22
        state['ph_real_fcc'] = -22
        state['ph_ocv'] = -22
        state['ph_rbatt'] = -22
        state['aged_begin_cc_uah'] = -22
        state['aged_begin_percent'] = -22
        state['aged_begin_ocv'] = -22
        state['aged_begin_cc_uah'] = -22
        state['aged_end_percent'] = -22
        state['aged_end_ocv'] = -22
        state['aged_end_cc_uah'] = -22

        adb_reboot()
        adb_update_charge_state()
    elif (i == 1):
        state['ph_real_fcc_batt_temp'] = random.randint(0, 50)
        print "--- Setting real_fcc_batt_temp =",state['ph_real_fcc_batt_temp']
        adb_update_charge_state()
    elif (i == 2):
        state['ph_real_fcc'] = random.randint(0, 4000000)
        print "--- Setting real_fcc =",state['ph_real_fcc']
        adb_update_charge_state()
    elif (i == 3):
        state['ph_ocv'] = random.randint(3200000, 4300000)
        print "--- Setting ocv =",state['ph_ocv']
        adb_update_charge_state()
    elif (i == 4):
        state['ph_rbatt'] = random.randint(1000, 2000)
        print "--- Setting rbatt =",state['ph_rbatt']
        adb_update_charge_state()

def test_case_random():
    global state
    global nvm_state

    pds_save_path = "random/"
    state_init()
    nvm_state = state.copy()

    max_cycle = input('Please enter number of cycles to execute: ')
    print "- Start random test case"
    adb_wait_for_device()
    execute_batt_health_reset()

    random.seed()

    for i in range(1, max_cycle):
        target_discharge = random.randint(0, state['ph_soc'])
        target_charge = random.randint(target_discharge, 100)

        print "- Executing cycle",i,"/",max_cycle
        execute_discharge_cycle(target_discharge, 10, 0.1)
        test_case_random_event()
        execute_charge_cycle(target_charge, 10, 0.1)
        test_case_random_event()

    adb_pull_bhd_data(pds_save_path)

    print "- Random test case completed!"
    print "- PDS data files saved in " + pds_save_path
    print "- Expected NVM State:"
    print "\tFile Write Count:\t\t",nvm_state['file_write_count']
    print "\tCharge Cycle Count:\t\t",nvm_state['charge_cycles']
    print "\tCharge Increase:\t\t",nvm_state['charge_inc']
    print "\tReal FCC Batt Temp:\t\t",nvm_state['ph_real_fcc_batt_temp']
    print "\tReal FCC:\t\t\t",nvm_state['ph_real_fcc']
    print "\tState of charge:\t\t",nvm_state['ph_soc']
    print "\tOCV:\t\t\t\t",nvm_state['ph_ocv']
    print "\tRbatt:\t\t\t\t",nvm_state['ph_rbatt']
    if (nvm_state['aged_begin_cc_uah'] != -22):
        print "\tAged Values:"
        print "\t\tBOC - Percent:\t\t",nvm_state['aged_begin_percent']
        print "\t\tBOC - OCV:\t\t",nvm_state['aged_begin_ocv']
        print "\t\tBOC - CC:\t\t",nvm_state['aged_begin_cc_uah']
        print "\t\tEOC - Percent:\t\t",nvm_state['aged_end_percent']
        print "\t\tEOC - OCV:\t\t",nvm_state['aged_end_ocv']
        print "\t\tEOC - CC:\t\t",nvm_state['aged_end_cc_uah']
    print

def test_case_charge_by_1_step():
    global state
    global nvm_state

    state_init()
    nvm_state = state.copy()

    print "- Start charge by 1 step test"
    adb_wait_for_device()
    execute_batt_health_reset()
    for i in range (0,5):
        execute_discharge_cycle(0, 1, 0.1)
        for j in range (0, 101):
            execute_charge_cycle(j, 1, 0.1)

    pds_save_path = "by_1/"
    adb_pull_bhd_data(pds_save_path)

    print "- Charge by 1 step test case completed!"
    print "- PDS data files saved in " + pds_save_path
    print "- Expected NVM State:"
    print "\tFile Write Count:\t\t",nvm_state['file_write_count']
    print "\tCharge Cycle Count:\t\t",nvm_state['charge_cycles']
    print "\tCharge Increase:\t\t",nvm_state['charge_inc']

def test_case_reboots():
    global state
    global nvm_state

    state_init()
    nvm_state = state.copy()

    print "- Start reboots test case"
    adb_wait_for_device()
    execute_batt_health_reset()
    execute_discharge_cycle(0, 10, 0.1)
    state['ph_real_fcc_batt_temp'] = 50
    execute_charge_cycle(50, 1, 0.1)
    adb_reboot()
    execute_discharge_cycle(50, 1, 0.1)
    state['ph_real_fcc'] = 1500000
    execute_charge_cycle(100, 1, 0.1)
    adb_reboot()
    execute_discharge_cycle(0, 1, 0.1)
    state['ph_ocv'] = 4000001
    execute_charge_cycle(50, 1, 0.1)
    adb_reboot()
    execute_discharge_cycle(50, 1, 0.1)
    state['ph_rbatt'] = 1400
    execute_charge_cycle(99, 1, 0.1)
    adb_reboot()

    pds_save_path = "reboots/"
    adb_pull_bhd_data(pds_save_path)

    print "- Reboots test case completed!"
    print "- PDS data files saved in " + pds_save_path
    print "- Expected NVM State:"
    print "\tFile Write Count:\t\t",nvm_state['file_write_count']
    print "\tCharge Cycle Count:\t\t",nvm_state['charge_cycles']
    print "\tCharge Increase:\t\t",nvm_state['charge_inc']
    print "\tReal FCC Batt Temp:\t\t",nvm_state['ph_real_fcc_batt_temp']
    print "\tReal FCC:\t\t\t",nvm_state['ph_real_fcc']
    print "\tState of charge:\t\t",nvm_state['ph_soc']
    print "\tOCV:\t\t\t\t",nvm_state['ph_ocv']
    print "\tRbatt:\t\t\t\t",nvm_state['ph_rbatt']
    print

def handle_force_nvm_write():
    global state
    global nvm_state

    #To force NVM write, just do a quick 1 -> 100 charge cycle
    state_init()
    nvm_state = state.copy()

    print "- Forcing NVM write"
    execute_discharge_cycle(1, 100, 0.1)
    state['ph_real_fcc_batt_temp'] = 50
    execute_charge_cycle(100, 100, 0.1)

def handle_manual_basic_entry():
    global state
    global nvm_state

    state_init()
    nvm_state = state.copy()

    print "- Manual entry, actions: "
    print "\t'=##' = charge/discharge to ##"
    print "\t'-##' = discharge by ##"
    print "\t'+##' = charge by ##"
    print "\tx = exit"
    exit_manual_entry = 0
    while not exit_manual_entry:
        choice = raw_input('Please enter action: ')
        if choice == 'x':
            exit_manual_entry = 1;
        else:
            soc = state['ph_soc'];
            choice.replace(' ', '')
            m = re.match('(=|-|\+)(\d*)', choice)
            if m:
                delta = eval(m.group(2))
                if (delta > 100): delta = 100
                if (delta < 0): delta = 0
                if (m.group(1) == '-'):
                    soc = soc - delta
                    if (soc < 0): soc = 0
                    execute_discharge_cycle(soc, 5, 0.1)
                elif (m.group(1) == '+'):
                    soc = soc + delta
                    if (soc > 100): soc = 100
                    execute_charge_cycle(soc, 5, 0.1)
                else:
                    if (soc < delta):
                        execute_charge_cycle(delta, 5, 0.1)
                    else:
                        execute_discharge_cycle(delta, 5, 0.1)
            else:
                print "Invalid input"


def handle_main_menu():
    exit_requested = 0
    print
    print "Battery Health Daemon Tester"
    print "============================"
    print "1) Execute 'Random' test"
    print "2) Execute 'Charge By 1' test"
    print "3) Execute 'Reboots' test"
    print "f) Force NVM write"
    print "m) Manual basic entry"
    print "r) Reset phone battery health data"
    print "x) Exit"
    choice = raw_input('Please enter a value: ')
    print

    if choice == '1':
        test_case_random()
    elif choice == '2':
        test_case_charge_by_1_step()
    elif choice == '3':
        test_case_reboots()
    elif choice == 'r':
        adb_wait_for_device()
        adb_stop_bhd()
        adb_clear_bhd_data()
        adb_reboot()
    elif choice == 'f':
        handle_force_nvm_write()
    elif choice == 'm':
        handle_manual_basic_entry()
    elif choice == 'x':
        exit_requested = 1
    else:
        print "Invalid value"

    return exit_requested


def main():
    try:
        exit_app = 0
        while (exit_app != 1):
            exit_app = handle_main_menu()

    except Exception, err:
        print 'ERROR: ' + str(err)
        traceback.print_exc()

if __name__ == '__main__':
    main()
