package com.nonlinearlabs.client.world.overlay.belt.parameters.recall;

import com.google.gwt.canvas.dom.client.Context2d;
import com.nonlinearlabs.client.Millimeter;
import com.nonlinearlabs.client.world.Dimension;
import com.nonlinearlabs.client.world.Gray;
import com.nonlinearlabs.client.world.RGB;
import com.nonlinearlabs.client.world.Rect;
import com.nonlinearlabs.client.world.overlay.OverlayControl;
import com.nonlinearlabs.client.world.overlay.OverlayLayout;

public abstract class RecallButton extends OverlayControl {
	boolean active;

	public RecallButton(OverlayLayout parent) {
		super(parent);
		active = false;
	}

	public boolean isActive() {
		return active;
	}

	public void setActive(boolean b) {
		active = b;
	}

	@Override
	public void draw(Context2d ctx, int invalidationMask) {
		if (isActive()) {
			getPixRect().drawRoundedRect(ctx, Rect.ROUNDING_ALL, Millimeter.toPixels(1), 1, new Gray(77),
					new Gray(77).brighter(15));
			drawTriangle(ctx, new Gray(77));
		}
	}

	private void drawTriangle(Context2d ctx, RGB color) {
		Rect movedToRight = getPixRect().copy().getMovedBy(new Dimension(getPixRect().getWidth(), 0)).copy();
		movedToRight = movedToRight.getReducedBy(movedToRight.getHeight() / 3);

		ctx.beginPath();
		ctx.setLineWidth(1);
		ctx.moveTo(movedToRight.getLeft(), movedToRight.getTop());
		ctx.lineTo(movedToRight.getLeft() + Millimeter.toPixels(3), movedToRight.getCenterPoint().getY());
		ctx.lineTo(movedToRight.getLeft(), movedToRight.getBottom());
		ctx.lineTo(movedToRight.getLeft(), movedToRight.getTop());
		ctx.setFillStyle(color.toString());
		ctx.setStrokeStyle(color.brighter(15).toString());
		ctx.closePath();
		ctx.fill();
		ctx.stroke();
	}
}