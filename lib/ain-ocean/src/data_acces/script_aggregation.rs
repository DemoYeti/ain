use anyhow::{anyhow, Error, Result};
use serde::{Deserialize, Serialize};
use serde_json;

use crate::{
    database::db_manager::{ColumnFamilyOperations, RocksDB},
    model::script_aggregation::ScriptAggregation,
};

pub struct ScriptAggretionDB {
    pub db: RocksDB,
}

impl ScriptAggretionDB {
    pub async fn query(
        &self,
        hid: String,
        limit: i32,
        lt: String,
    ) -> Result<Vec<ScriptAggregation>> {
        todo!()
    }
    pub async fn store(&self, aggregation: ScriptAggregation) -> Result<()> {
        match serde_json::to_string(&aggregation) {
            Ok(value) => {
                let key = aggregation.hid.clone();
                self.db
                    .put("script_aggregation", key.as_bytes(), value.as_bytes())?;
                Ok(())
            }
            Err(e) => Err(anyhow!(e)),
        }
    }
    pub async fn get(&self, hid: String) -> Result<Option<ScriptAggregation>> {
        match self.db.get("script_aggregation", hid.as_bytes()) {
            Ok(Some(value)) => {
                let oracle: ScriptAggregation =
                    serde_json::from_slice(&value).map_err(|e| anyhow!(e))?;
                Ok(Some(oracle))
            }
            Ok(None) => Ok(None),
            Err(e) => Err(anyhow!(e)),
        }
    }
    pub async fn delete(&self, hid: String) -> Result<()> {
        match self.db.delete("script_aggregation", hid.as_bytes()) {
            Ok(_) => Ok(()),
            Err(e) => Err(anyhow!(e)),
        }
    }
}
