package com.nonlinearlabs.NonMaps.client.world.maps.presets;

import com.nonlinearlabs.NonMaps.client.NonMaps;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.preset.Preset;

public class ShiftMultiplePresetSelection extends MultiplePresetSelection {

	public ShiftMultiplePresetSelection(Preset p) {
		super(p);
	}

	public void toggle(Preset p) {
		if (NonMaps.get().getNonLinearWorld().isShiftDown()) {
			super.toggle(p);
		} else {
			NonMaps.get().getNonLinearWorld().getPresetManager().closeMultiSelection();
			p.select();
		}
	}
}
