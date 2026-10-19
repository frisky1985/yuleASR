<template>
  <div class="snapshot-manager">
    <div class="snapshot-header">
      <h3>Configuration Snapshots</h3>
      <el-button type="primary" @click="showSaveDialog = true">
        <el-icon><Plus /></el-icon> Save Snapshot
      </el-button>
    </div>

    <div v-if="snapshotStore.sortedSnapshots.length === 0" class="empty-state">
      <el-empty description="No snapshots yet. Save your first snapshot to track configuration changes." />
    </div>

    <div v-else class="snapshot-list">
      <div
        v-for="snapshot in snapshotStore.sortedSnapshots"
        :key="snapshot.id"
        class="snapshot-item"
        :class="{ active: snapshot.id === snapshotStore.currentSnapshotId }"
      >
        <div class="snapshot-info">
          <div class="snapshot-name">{{ snapshot.name }}</div>
          <div class="snapshot-meta">
            <span class="snapshot-time">{{ formatTime(snapshot.timestamp) }}</span>
            <span v-if="snapshot.description" class="snapshot-desc">{{ snapshot.description }}</span>
          </div>
          <div v-if="snapshot.tags.length > 0" class="snapshot-tags">
            <el-tag v-for="tag in snapshot.tags" :key="tag" size="small" type="info">{{ tag }}</el-tag>
          </div>
        </div>
        <div class="snapshot-actions">
          <el-button size="small" type="primary" text @click="handleRestore(snapshot.id)">
            <el-icon><RefreshLeft /></el-icon> Restore
          </el-button>
          <el-button size="small" type="danger" text @click="handleDelete(snapshot.id)">
            <el-icon><Delete /></el-icon> Delete
          </el-button>
        </div>
      </div>
    </div>

    <!-- Save Snapshot Dialog -->
    <el-dialog v-model="showSaveDialog" title="Save Snapshot" width="500px">
      <el-form :model="saveForm" label-width="100px">
        <el-form-item label="Name" required>
          <el-input v-model="saveForm.name" placeholder="e.g., Before MCAL changes" />
        </el-form-item>
        <el-form-item label="Description">
          <el-input
            v-model="saveForm.description"
            type="textarea"
            :rows="3"
            placeholder="Optional description of what changed"
          />
        </el-form-item>
        <el-form-item label="Tags">
          <el-select v-model="saveForm.tags" multiple placeholder="Add tags" style="width: 100%">
            <el-option label="baseline" value="baseline" />
            <el-option label="before-release" value="before-release" />
            <el-option label="experimental" value="experimental" />
            <el-option label="stable" value="stable" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSaveDialog = false">Cancel</el-button>
        <el-button type="primary" @click="confirmSave" :disabled="!saveForm.name.trim()">Save</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { Plus, RefreshLeft, Delete } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useSnapshotStore } from '@/stores/snapshot'

const snapshotStore = useSnapshotStore()

const showSaveDialog = ref(false)
const saveForm = ref({
  name: '',
  description: '',
  tags: [] as string[],
})

onMounted(() => {
  snapshotStore.loadSnapshots()
})

function formatTime(timestamp: number): string {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - timestamp

  if (diff < 60000) return 'Just now'
  if (diff < 3600000) return `${Math.floor(diff / 60000)} minutes ago`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} hours ago`
  if (diff < 604800000) return `${Math.floor(diff / 86400000)} days ago`

  return date.toLocaleDateString() + ' ' + date.toLocaleTimeString()
}

function handleRestore(snapshotId: string) {
  ElMessageBox.confirm(
    'This will replace your current configuration with the snapshot. Continue?',
    'Restore Snapshot',
    { confirmButtonText: 'Restore', cancelButtonText: 'Cancel', type: 'warning' }
  ).then(() => {
    const success = snapshotStore.restoreSnapshot(snapshotId)
    if (success) {
      ElMessage.success('Snapshot restored')
    } else {
      ElMessage.error('Failed to restore snapshot')
    }
  }).catch(() => {})
}

function handleDelete(snapshotId: string) {
  ElMessageBox.confirm(
    'This will permanently delete the snapshot. Continue?',
    'Delete Snapshot',
    { confirmButtonText: 'Delete', cancelButtonText: 'Cancel', type: 'warning' }
  ).then(() => {
    const success = snapshotStore.deleteSnapshot(snapshotId)
    if (success) {
      ElMessage.success('Snapshot deleted')
    } else {
      ElMessage.error('Failed to delete snapshot')
    }
  }).catch(() => {})
}

function confirmSave() {
  const snapshot = snapshotStore.saveSnapshot(
    saveForm.value.name.trim(),
    saveForm.value.description.trim(),
    saveForm.value.tags
  )

  if (snapshot) {
    ElMessage.success('Snapshot saved')
    showSaveDialog.value = false
    saveForm.value = { name: '', description: '', tags: [] }
  } else {
    ElMessage.error('Failed to save snapshot')
  }
}
</script>

<style scoped>
.snapshot-manager {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fff;
}

.snapshot-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 20px;
  border-bottom: 1px solid #e4e7ed;
}

.snapshot-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
}

.empty-state {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.snapshot-list {
  flex: 1;
  overflow-y: auto;
  padding: 16px 20px;
}

.snapshot-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  margin-bottom: 8px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  transition: all 0.2s;
}

.snapshot-item:hover {
  border-color: #409eff;
  background: #f5f7fa;
}

.snapshot-item.active {
  border-color: #409eff;
  background: #ecf5ff;
}

.snapshot-info {
  flex: 1;
  min-width: 0;
}

.snapshot-name {
  font-weight: 500;
  color: #303133;
  margin-bottom: 4px;
}

.snapshot-meta {
  display: flex;
  gap: 12px;
  font-size: 12px;
  color: #909399;
  margin-bottom: 4px;
}

.snapshot-tags {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.snapshot-actions {
  display: flex;
  gap: 8px;
  flex-shrink: 0;
}
</style>
