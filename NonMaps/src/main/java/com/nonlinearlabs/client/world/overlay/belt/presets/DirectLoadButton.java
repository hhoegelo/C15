package com.nonlinearlabs.client.world.overlay.belt.presets;

import com.google.gwt.xml.client.Node;
import com.nonlinearlabs.client.Millimeter;
import com.nonlinearlabs.client.NonMaps;
import com.nonlinearlabs.client.ServerProxy;
import com.nonlinearlabs.client.world.Control;
import com.nonlinearlabs.client.world.Position;
import com.nonlinearlabs.client.world.overlay.OverlayLayout;
import com.nonlinearlabs.client.world.overlay.SVGImage;

public class DirectLoadButton extends SVGImage {

	private boolean married;

	DirectLoadButton(OverlayLayout parent) {
		super(parent, "Link_Enabled.svg", "Link_Active.svg", "Link_Disabled.svg");
	}

	public boolean isInStoreSelectMode() {
		return NonMaps.get().getNonLinearWorld().getPresetManager().isInStoreSelectMode();
	}

	@Override
	public void doLayout(double x, double y, double w, double h) {
		double imgHeight = getSelectedImage().getImgHeight();
		y = (h - imgHeight) / 2;
		y += Millimeter.toPixels(3);
		h = imgHeight;
		super.doLayout(x, y, w, h);
	}

	@Override
	public int getSelectedPhase() {
		if (isInStoreSelectMode())
			return drawStates.disabled.ordinal();
		else if (married)
			return drawStates.active.ordinal();

		return drawStates.normal.ordinal();
	}

	@Override
	public Control mouseDown(Position eventPoint) {
		if (isInStoreSelectMode())
			return this;

		toggle();
		return this;
	}

	public void update(Node settingsNode) {
		if (ServerProxy.didChange(settingsNode)) {
			String str = ServerProxy.getChildText(settingsNode, "LoadMode", "value");
			if (str != null && !str.isEmpty()) {
				boolean m = str.toLowerCase().equals("on");
				if (married != m) {
					married = m;
					invalidate(INVALIDATION_FLAG_UI_CHANGED);
					requestLayout();
				}
			}
		}
	}

	public boolean isDirectLoadActive() {
		return married;
	}

	public void toggle() {
		married = !married;
		NonMaps.theMaps.getServerProxy().setSetting("LoadMode", married ? "on" : "off");
	}
}
