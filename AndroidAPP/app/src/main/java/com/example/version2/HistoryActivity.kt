package com.example.version2

import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.widget.Button
import android.widget.TableLayout
import android.widget.TableRow
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet

class HistoryActivity : AppCompatActivity() {

    private lateinit var historyTable: TableLayout
    private lateinit var backButton: Button
    private lateinit var lineChart: LineChart

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_history)

        historyTable = findViewById(R.id.history_table)
        backButton = findViewById(R.id.back_button)
        lineChart = findViewById(R.id.line_chart)

        backButton.setOnClickListener {
            finish()
        }

        displayHistoryAndChart()
    }

    private fun displayHistoryAndChart() {
        val history = HistoryManager.getHistory(this).reversed() // Reverse to show oldest to newest

        // --- Populate Table ---
        val childCount = historyTable.childCount
        if (childCount > 1) {
            historyTable.removeViews(1, childCount - 1)
        }

        for ((index, entry) in history.withIndex()) {
            val totalTime = entry.goodPostureTime + entry.badPostureTime
            val goodPercentage = if (totalTime > 0) (entry.goodPostureTime * 100) / totalTime else 0
            val badPercentage = if (totalTime > 0) 100 - goodPercentage else 0

            val row = TableRow(this).apply {
                // Zebra striping
                if (index % 2 == 0) {
                    setBackgroundColor(ContextCompat.getColor(context, R.color.md_theme_surfaceVariant))
                } else {
                    setBackgroundColor(Color.TRANSPARENT)
                }
            }

            val sessionTextView = createTableCell((index + 1).toString())
            val totalTimeTextView = createTableCell(getString(R.string.time_format_seconds, totalTime))
            val goodTimeTextView = createTableCell(getString(R.string.time_format_seconds, entry.goodPostureTime))
            val badTimeTextView = createTableCell(getString(R.string.time_format_seconds, entry.badPostureTime))
            val goodPercentageTextView = createTableCell(getString(R.string.percentage_format_table, goodPercentage))
            val badPercentageTextView = createTableCell(getString(R.string.percentage_format_table, badPercentage))

            row.addView(sessionTextView)
            row.addView(totalTimeTextView)
            row.addView(goodTimeTextView)
            row.addView(badTimeTextView)
            row.addView(goodPercentageTextView)
            row.addView(badPercentageTextView)

            historyTable.addView(row)
        }

        // --- Setup Chart ---
        if (history.isNotEmpty()) {
            setupLineChart(history)
        }
    }

    private fun createTableCell(text: String): TextView {
        return TextView(this).apply {
            this.text = text
            gravity = Gravity.CENTER
            setPadding(8, 16, 8, 16)
        }
    }

    private fun setupLineChart(history: List<HistoryManager.HistoryEntry>) {
        val entries = ArrayList<Entry>()
        for ((index, entry) in history.withIndex()) {
            val totalTime = entry.goodPostureTime + entry.badPostureTime
            val goodPercentage = if (totalTime > 0) (entry.goodPostureTime * 100f) / totalTime else 0f
            entries.add(Entry(index.toFloat(), goodPercentage))
        }

        val dataSet = LineDataSet(entries, "Good Posture %").apply {
            color = ContextCompat.getColor(this@HistoryActivity, R.color.md_theme_primary)
            valueTextColor = ContextCompat.getColor(this@HistoryActivity, R.color.md_theme_onSurface)
            setCircleColor(ContextCompat.getColor(this@HistoryActivity, R.color.md_theme_primary))
            lineWidth = 2.5f
            circleRadius = 4.5f
            setDrawCircleHole(false)
            valueTextSize = 10f
            fillColor = ContextCompat.getColor(this@HistoryActivity, R.color.md_theme_primaryContainer)
            setDrawFilled(true)
        }

        val lineData = LineData(dataSet)
        lineChart.data = lineData

        // Customize Chart
        lineChart.description.isEnabled = false
        lineChart.legend.textSize = 12f
        lineChart.animateX(1000)

        // Customize X-axis
        val xAxis = lineChart.xAxis
        xAxis.position = XAxis.XAxisPosition.BOTTOM
        xAxis.granularity = 1f
        xAxis.setDrawGridLines(false)

        // Customize Y-axis
        val leftAxis = lineChart.axisLeft
        leftAxis.axisMinimum = 0f
        leftAxis.axisMaximum = 100f

        lineChart.axisRight.isEnabled = false

        lineChart.invalidate() // Refresh chart
    }
}
