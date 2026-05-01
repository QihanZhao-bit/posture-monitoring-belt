package com.example.version2

import android.content.Context
import android.content.SharedPreferences

object HistoryManager {
    private const val PREFS_NAME = "PostureHistory"
    private const val MAX_HISTORY_SIZE = 10

    data class HistoryEntry(val goodPostureTime: Long, val badPostureTime: Long)

    fun saveHistory(context: Context, goodTime: Long, badTime: Long) {
        val sharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val history = getHistory(context).toMutableList()

        history.add(0, HistoryEntry(goodTime, badTime))
        if (history.size > MAX_HISTORY_SIZE) {
            history.removeAt(history.size - 1)
        }

        with(sharedPreferences.edit()) {
            putInt("history_size", history.size)
            for (i in history.indices) {
                putString("history_${i}_good", history[i].goodPostureTime.toString())
                putString("history_${i}_bad", history[i].badPostureTime.toString())
            }
            apply()
        }
    }

    fun getHistory(context: Context): List<HistoryEntry> {
        val sharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val size = sharedPreferences.getInt("history_size", 0)
        val history = mutableListOf<HistoryEntry>()

        for (i in 0 until size) {
            val goodTime = sharedPreferences.getString("history_${i}_good", "0")?.toLong() ?: 0L
            val badTime = sharedPreferences.getString("history_${i}_bad", "0")?.toLong() ?: 0L
            history.add(HistoryEntry(goodTime, badTime))
        }

        return history
    }
}