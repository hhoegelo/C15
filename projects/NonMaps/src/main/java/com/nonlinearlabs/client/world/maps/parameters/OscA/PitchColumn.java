package com.nonlinearlabs.client.world.maps.parameters.OscA;

import com.nonlinearlabs.client.world.maps.MapsLayout;
import com.nonlinearlabs.client.world.maps.parameters.KeyTrackParameter;
import com.nonlinearlabs.client.world.maps.parameters.LabelModulationSource;
import com.nonlinearlabs.client.world.maps.parameters.ModulatableParameter;
import com.nonlinearlabs.client.world.maps.parameters.ModulateableNumericalControl;
import com.nonlinearlabs.client.world.maps.parameters.ModulationSourceHighPriority;
import com.nonlinearlabs.client.world.maps.parameters.ModulationSourceSlider;
import com.nonlinearlabs.client.world.maps.parameters.ParameterColumn;

class PitchColumn extends ParameterColumn {

	private class Pitch extends ModulatableParameter {

		private Pitch(MapsLayout parent) {
			super(parent, 53);
			addChild(new ModulateableNumericalControl(this, getParameterNumber()));
		}

		@Override
		public double getPadding() {
			return 2;
		}
	}

	private class Keytracking extends ModulationSourceHighPriority {

		private Keytracking(MapsLayout parent) {
			super(parent, 55);
			addChild(new LabelModulationSource(this, getParameterNumber()));
			addChild(new KeyTrackParameter(this, getParameterNumber()));
		}

	}

	PitchColumn(MapsLayout parent) {
		super(parent);
		addChild(new Pitch(this));
		addChild(new ModulationSourceSlider(this, 56));
		addChild(new Keytracking(this));
	}
}
