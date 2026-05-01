package com.example.version2

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.media.RingtoneManager
import android.os.Build
import android.os.Bundle
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.view.View
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    private lateinit var statusTextView: TextView
    private lateinit var connectButton: Button
    private lateinit var historyButton: Button
    private lateinit var goodPostureTimeTextView: TextView
    private lateinit var badPostureTimeTextView: TextView
    private lateinit var goodPosturePercentageTextView: TextView
    private lateinit var badPosturePercentageTextView: TextView

    private var isConnected = false
    private var goodPostureTime = 0L
    private var badPostureTime = 0L
    private var continuousBadPostureTime = 0L
    private var timerJob: Job? = null

    private lateinit var bleManager: BleManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusTextView = findViewById(R.id.status_textview)
        connectButton = findViewById(R.id.connect_button)
        historyButton = findViewById(R.id.history_button)
        goodPostureTimeTextView = findViewById(R.id.good_posture_time)
        badPostureTimeTextView = findViewById(R.id.bad_posture_time)
        goodPosturePercentageTextView = findViewById(R.id.good_posture_percentage)
        badPosturePercentageTextView = findViewById(R.id.bad_posture_percentage)

        bleManager = BleManager(this, 
            onDataReceived = { data ->
                runOnUiThread {
                    statusTextView.text = data
                }
            },
            onStatusChanged = { status ->
                runOnUiThread {
                    statusTextView.text = status
                    if (status == "Monitoring Posture") {
                        isConnected = true
                        connectButton.text = getString(R.string.disconnect)
                        startTimer()
                    } else if (status == "Disconnected") {
                        handleDisconnect()
                    }
                }
            }
        )

        connectButton.setOnClickListener {
            if (isConnected) {
                bleManager.disconnect()
            } else {
                if (checkPermissions()) {
                    startBleConnection()
                } else {
                    requestPermissions()
                }
            }
        }

        historyButton.setOnClickListener {
            val intent = Intent(this, HistoryActivity::class.java)
            startActivity(intent)
        }
    }

    private fun startBleConnection() {
        goodPostureTime = 0L
        badPostureTime = 0L
        continuousBadPostureTime = 0L
        updateUI()
        bleManager.startScan()
    }

    private fun handleDisconnect() {
        isConnected = false
        stopTimer()
        connectButton.text = getString(R.string.connect)
        
        if (goodPostureTime > 0 || badPostureTime > 0) {
            HistoryManager.saveHistory(this, goodPostureTime, badPostureTime)
        }
    }

    private fun updateUI() {
        goodPostureTimeTextView.text = getString(R.string.initial_time)
        badPostureTimeTextView.text = getString(R.string.initial_time)
        goodPosturePercentageTextView.text = getString(R.string.initial_percentage)
        badPosturePercentageTextView.text = getString(R.string.initial_percentage)
    }

    private fun startTimer() {
        timerJob?.cancel()
        timerJob = lifecycleScope.launch {
            while (isConnected) {
                delay(1000)
                val currentStatus = statusTextView.text.toString().lowercase()
                
                if (currentStatus.contains("good")) {
                    goodPostureTime++
                    continuousBadPostureTime = 0L
                } else if (currentStatus.contains("bad")) {
                    badPostureTime++
                    continuousBadPostureTime++
                    
                    if (continuousBadPostureTime >= 180) {
                        triggerAlert()
                        continuousBadPostureTime = 0L
                    }
                }
                
                runOnUiThread {
                    goodPostureTimeTextView.text = getString(R.string.time_format_seconds, goodPostureTime)
                    badPostureTimeTextView.text = getString(R.string.time_format_seconds, badPostureTime)

                    val totalTime = goodPostureTime + badPostureTime
                    if (totalTime > 0) {
                        val goodPercentage = (goodPostureTime * 100) / totalTime
                        goodPosturePercentageTextView.text = getString(R.string.percentage_format, goodPercentage)
                        badPosturePercentageTextView.text = getString(R.string.percentage_format, 100 - goodPercentage)
                    }
                }
            }
        }
    }

    private fun triggerAlert() {
        val audioManager = getSystemService(Context.AUDIO_SERVICE) as AudioManager
        if (audioManager.ringerMode == AudioManager.RINGER_MODE_NORMAL) {
            try {
                val notification = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)
                val r = RingtoneManager.getRingtone(applicationContext, notification)
                r.play()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        } else {
            val vibrator = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                val vibratorManager = getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as VibratorManager
                vibratorManager.defaultVibrator
            } else {
                @Suppress("DEPRECATION")
                getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
            }
            vibrator.vibrate(VibrationEffect.createOneShot(1000, VibrationEffect.DEFAULT_AMPLITUDE))
        }
    }

    private fun stopTimer() {
        timerJob?.cancel()
    }

    private fun checkPermissions(): Boolean {
        val permissions = mutableListOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        }
        return permissions.all { ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED }
    }

    private fun requestPermissions() {
        val permissions = mutableListOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        }
        ActivityCompat.requestPermissions(this, permissions.toTypedArray(), 1)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1 && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
            startBleConnection()
        } else {
            Toast.makeText(this, getString(R.string.alert_permissions_required), Toast.LENGTH_SHORT).show()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        bleManager.disconnect()
    }
}