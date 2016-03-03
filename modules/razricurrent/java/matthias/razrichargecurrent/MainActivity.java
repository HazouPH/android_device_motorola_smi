package matthias.razrichargecurrent;

import android.content.Intent;
import android.provider.MediaStore;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.RadioGroup;

import java.io.FileInputStream;
import java.io.FileOutputStream;


public class MainActivity extends ActionBarActivity {
    private static final String MSIC_CURRENT_FILE = "/sys/devices/ipc/msic_battery/charge_enable";

    private void updateSettings() {
        try {
            FileInputStream fis = new FileInputStream(MainActivity.MSIC_CURRENT_FILE);
            int i = fis.read();
            fis.close();
            i -= '0';
            if(i < 0 || i > 6)
                return;

            if(i < 4) {
                ((RadioGroup)findViewById(R.id.grp_curr)).check(R.id.rad_lim_500);
            } else if(i == 4) {
                ((RadioGroup)findViewById(R.id.grp_curr)).check(R.id.rad_lim_no);
            } else if(i == 5) {
                ((RadioGroup)findViewById(R.id.grp_curr)).check(R.id.rad_ovr_950);
            } else if(i == 6) {
                ((RadioGroup)findViewById(R.id.grp_curr)).check(R.id.rad_ovr_inf);
            }

        } catch (Exception e) {
            // IO error stuff
        }
    }

    private static void setSettings(int v) {
        StringBuilder sb = new StringBuilder();
        sb.append("echo ");
        sb.append(v);
        sb.append(" > ");
        sb.append(MainActivity.MSIC_CURRENT_FILE);
        ExecuteAsRoot.execute(sb.toString());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ((RadioGroup) findViewById(R.id.grp_curr)).
                setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                                               public void onCheckedChanged(RadioGroup group, int checkedId) {
                                                   int v = 0;
                                                   switch (checkedId) {
                                                       case R.id.rad_lim_no:
                                                           v = 4;
                                                           break;
                                                       case R.id.rad_lim_500:
                                                           v = 3;
                                                           break;
                                                       case R.id.rad_ovr_950:
                                                           v = 5;
                                                           break;
                                                       case R.id.rad_ovr_inf:
                                                           v = 6;
                                                           break;
                                                   }
                                                   MainActivity.setSettings(v);
                                               }
                                           }
                );
    }

    @Override
    protected void onResume() {
        super.onResume();
        this.updateSettings();
    }

}
