package com.nonlinearlabs.client.world.maps.parameters.FBMixer;

import com.nonlinearlabs.client.world.Rect;
import com.nonlinearlabs.client.world.maps.MapsLayout;
import com.nonlinearlabs.client.world.maps.parameters.DoubleParameterColumn;
import com.nonlinearlabs.client.world.maps.parameters.KnobSmall;
import com.nonlinearlabs.client.world.maps.parameters.ModulateableKnob;
import com.nonlinearlabs.client.world.maps.parameters.Parameter;
import com.nonlinearlabs.client.world.maps.parameters.ParameterColumn;
import com.nonlinearlabs.client.world.maps.parameters.SizeLinkedParameterColumn;
import com.nonlinearlabs.client.world.maps.parameters.SmallParameterName;
import com.nonlinearlabs.client.world.maps.parameters.ValueDisplaySmall;

class DriveColumn extends DoubleParameterColumn {

	class DriveParameterColumn extends SizeLinkedParameterColumn {

		private class Drive extends ModulateableKnob {

			private Drive(MapsLayout parent) {
				super(parent, 164);
			}

			@Override
			protected int getBackgroundRoundings() {
				return Rect.ROUNDING_LEFT;
			}
		}

		public DriveParameterColumn(MapsLayout parent) {
			super(parent);
			addChild(new Drive(this));
		}
	}

	class FoldAndAsymetryColumn extends ParameterColumn {

		private class Fold extends Parameter {

			private Fold(MapsLayout parent) {
				super(parent, 166);
				addChild(new SmallParameterName(this, getName()));
				addChild(new KnobSmall(this));
				addChild(new ValueDisplaySmall(this));
			}

			@Override
			protected int getBackgroundRoundings() {
				return Rect.ROUNDING_RIGHT_TOP;
			}
		}

		private class Asym extends Parameter {

			private Asym(MapsLayout parent) {
				super(parent, 167);
				addChild(new SmallParameterName(this, getName()));
				addChild(new KnobSmall(this));
				addChild(new ValueDisplaySmall(this));
			}

			@Override
			protected int getBackgroundRoundings() {
				return Rect.ROUNDING_RIGHT_BOTTOM;
			}
		}

		public FoldAndAsymetryColumn(MapsLayout parent) {
			super(parent);
			addChild(new Fold(this));
			addChild(new Asym(this));
		}
	}

	DriveColumn(MapsLayout parent) {
		super(parent);
		addChild(new DriveParameterColumn(this));
		addChild(new FoldAndAsymetryColumn(this));
	}
}
