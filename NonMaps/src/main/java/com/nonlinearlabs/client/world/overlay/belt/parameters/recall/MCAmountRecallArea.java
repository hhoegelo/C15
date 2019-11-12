package com.nonlinearlabs.client.world.overlay.belt.parameters.recall;

import com.nonlinearlabs.client.dataModel.editBuffer.BasicParameterModel;
import com.nonlinearlabs.client.dataModel.editBuffer.EditBufferModel;
import com.nonlinearlabs.client.dataModel.editBuffer.ModulateableParameterModel;
import com.nonlinearlabs.client.world.overlay.belt.parameters.BeltParameterLayout;
import com.nonlinearlabs.client.world.overlay.belt.parameters.BeltParameterLayout.Mode;

public class MCAmountRecallArea extends RecallArea {

	public MCAmountRecallArea(BeltParameterLayout parent) {
		super(parent);
		addChild(button = new MCAmountRecallButton(this));
		addChild(value = new MCAmountRecallValue(this));
	}

	@Override
	public boolean isChanged() {
		BasicParameterModel bpm = EditBufferModel.getSelectedParameter();
		if (bpm instanceof ModulateableParameterModel) {
			return ((ModulateableParameterModel) bpm).isModAmountChanged();
		}
		return false;
	}

	@Override
	public void setVisibleForMode(Mode mode) {
		setVisible(mode == Mode.mcAmount);
	}
}
