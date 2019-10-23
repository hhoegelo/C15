package com.nonlinearlabs.client.world.maps.parameters.Flanger;

import com.nonlinearlabs.client.world.maps.MapsLayout;
import com.nonlinearlabs.client.world.maps.parameters.ModulateableKnob;
import com.nonlinearlabs.client.world.maps.parameters.ParameterColumn;

class FlangerCol5 extends ParameterColumn {

	FlangerCol5(MapsLayout parent) {
		super(parent);
		addChild(new ModulateableKnob(this, 310));
	}
}
