// Baza Room — jeden rekord na save (aktywna kariera).
// W przyszłości dojdzie tabela sezonów do historii, ale w wersji 0.x trzymamy
// ją po stronie rdzenia (w save JSON).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.data

import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Database
import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.RoomDatabase
import kotlinx.coroutines.flow.Flow

@Entity(tableName = "career_save")
data class CareerSaveEntity(
    @PrimaryKey val slotId: Int = 0,
    @ColumnInfo(name = "json") val json: String,
    @ColumnInfo(name = "saved_at_epoch_ms") val savedAtEpochMs: Long,
)

@Dao
interface CareerDao {
    @Query("SELECT * FROM career_save WHERE slotId = 0 LIMIT 1")
    suspend fun load(): CareerSaveEntity?

    @Query("SELECT * FROM career_save WHERE slotId = 0 LIMIT 1")
    fun loadFlow(): Flow<CareerSaveEntity?>

    @Query("INSERT OR REPLACE INTO career_save (slotId, json, saved_at_epoch_ms) VALUES (0, :json, :savedAtEpochMs)")
    suspend fun save(json: String, savedAtEpochMs: Long)

    @Query("DELETE FROM career_save WHERE slotId = 0")
    suspend fun clear()
}

@Database(
    entities = [CareerSaveEntity::class],
    version = 1,
    exportSchema = false,
)
abstract class AppDatabase : RoomDatabase() {
    abstract fun careerDao(): CareerDao
}