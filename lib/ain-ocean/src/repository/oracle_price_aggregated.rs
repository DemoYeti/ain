use std::sync::Arc;

use ain_db::LedgerColumn;
use ain_macros::Repository;

use super::RepositoryOps;
use crate::{
    model::OraclePriceAggregated,
    storage::{columns, ocean_store::OceanStore},
    Result,
};

#[derive(Repository)]
#[repository(K = "String", V = "String")]
pub struct OraclePriceAggregatedRepository {
    pub store: Arc<OceanStore>,
    col: LedgerColumn<columns::OraclePriceAggregated>,
}

impl OraclePriceAggregatedRepository {
    pub fn new(store: Arc<OceanStore>) -> Self {
        Self {
            col: store.column(),
            store,
        }
    }
}
