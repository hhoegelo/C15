package com.nonlinearlabs.client.world.overlay;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import com.google.gwt.canvas.dom.client.Context2d;
import com.google.gwt.xml.client.Node;
import com.nonlinearlabs.client.ColorTable;
import com.nonlinearlabs.client.Millimeter;
import com.nonlinearlabs.client.NonMaps;
import com.nonlinearlabs.client.dataModel.editBuffer.EditBufferModel;
import com.nonlinearlabs.client.dataModel.editBuffer.EditBufferModel.SoundType;
import com.nonlinearlabs.client.presenters.EditBufferPresenterProvider;
import com.nonlinearlabs.client.useCases.EditBufferUseCases;
import com.nonlinearlabs.client.world.Control;
import com.nonlinearlabs.client.world.Gray;
import com.nonlinearlabs.client.world.Position;
import com.nonlinearlabs.client.world.RGB;
import com.nonlinearlabs.client.world.RGBA;
import com.nonlinearlabs.client.world.Rect;
import com.nonlinearlabs.client.world.Viewport;
import com.nonlinearlabs.client.world.overlay.belt.Belt;
import com.nonlinearlabs.client.world.overlay.belt.presets.PresetColorTagContextMenu;
import com.nonlinearlabs.client.world.overlay.menu.GlobalMenu;
import com.nonlinearlabs.client.world.overlay.undo.UndoTreeWindow;
import com.nonlinearlabs.client.world.pointer.Down;
import com.nonlinearlabs.client.world.pointer.Gesture;

public class Overlay extends OverlayLayout {

	private class LayerDisplay extends Label {

		public LayerDisplay(OverlayLayout parent) {
			super(parent);

			EditBufferModel.get().soundType.onChange(v -> {
				setVisible(v != SoundType.Single);
				return true;
			});
		}

		@Override
		public String getDrawText(Context2d ctx) {
			return EditBufferPresenterProvider.getPresenter().voiceGroup;
		}

		@Override
		protected void drawText(Context2d ctx, String text, Position left) {
			ctx.setStrokeStyle(RGB.black().toString());
			ctx.strokeText(text, left.getX(), left.getY() + getVerticalFontDisplacement());
			ctx.setFillStyle(EditBufferPresenterProvider.getPresenter().voiceGroupIndicationColor.toString());
			ctx.fillText(text, left.getX(), left.getY() + getVerticalFontDisplacement());
		}

		@Override
		protected double getFontHeight(Rect pixRect) {
			return super.getFontHeight(pixRect) * 2;
		}

		@Override
		public void draw(Context2d ctx, int invalidationMask) {
			RGB c = new RGBA(EditBufferPresenterProvider.getPresenter().voiceGroupIndicationColor, 0.25);
			getPixRect().fill(ctx, c);
			super.draw(ctx, invalidationMask);
		}

		@Override
		public Control mouseDown(Position eventPoint) {
			if (isVisible()) 
				return this;
			return super.mouseDown(eventPoint);
		}

		@Override
		public Control click(Position eventPoint) {
			if (isVisible()) {
				EditBufferUseCases.get().toggleVoiceGroup();
				return this;
			}

			return super.click(eventPoint);
		}

	}

	private Belt belt = null;
	private GlobalButtons buttons = null;
	private UndoRedoButtons undoRedo = null;
	private boolean layoutRequested = false;
	private boolean initialLayout = true;
	private GlobalMenu globalMenu;
	private UndoTreeWindow undo;
	private List<CompareDialog> compareDialogs;
	private ModalDialog modalDialog;
	private LayerDisplay layerDisplay;

	public Overlay(Viewport parent) {
		super(parent);
		addChild(layerDisplay = new LayerDisplay(this));
		addChild(belt = new Belt(this, parent.getNonMaps()));
		addChild(buttons = new GlobalButtons(this, belt));
		addChild(undoRedo = new UndoRedoButtons(this, belt));
		addChild(undo = new UndoTreeWindow(this));

		// has to be the last one to add as it refers to previously added
		// members
		addChild(globalMenu = new GlobalMenu(this));

		compareDialogs = new ArrayList<CompareDialog>();
	}

	public void refreshGlobalMenu() {
		globalMenu.refresh();
	}

	@Override
	public Viewport getParent() {
		return (Viewport) super.getParent();
	}

	@Override
	public NonMaps getNonMaps() {
		return getParent().getNonMaps();
	}

	@Override
	public boolean isRectVisible(Rect r) {
		return true;
	}

	@Override
	public void requestLayout() {
		layoutRequested = true;
		invalidate(INVALIDATION_FLAG_UI_CHANGED);
	}

	public void promptUser(String prompt, Runnable okAction, Runnable cancelAction) {
		modalDialog = null;
		modalDialog = new ModalDialog(prompt, okAction, cancelAction);
		modalDialog.initalShow();
	}

	@Override
	public void draw(Context2d ctx, int invalidationMask) {
		if (layoutRequested) {
			layoutRequested = false;
			doLayout(0, 0, getRelativePosition().getWidth(), getRelativePosition().getHeight());
		}

		buttons.drawInactiveButton(ctx, invalidationMask);
		drawBackground(ctx);
		buttons.drawActiveButton(ctx, invalidationMask);

		if (EditBufferModel.get().soundType.getValue() != SoundType.Single)
			drawDualSoundIndication(ctx);

		super.draw(ctx, invalidationMask);
	}

	private void drawDualSoundIndication(Context2d ctx) {
		Rect r = getPixRect().copy();
		r.setBottom(belt.getPixRect().getTop());

		Rect gbr = buttons.getPixRect();
		Rect ldr = layerDisplay.getPixRect();

		double w = 2;
		double corner = Millimeter.toPixels(1);
		r = r.getReducedBy(w / 1);

		ctx.beginPath();
		ctx.moveTo(r.getLeft() + corner, r.getTop());
		ctx.lineTo(ldr.getLeft() - corner, r.getTop());
		ctx.arcTo(ldr.getLeft(), ldr.getTop(), ldr.getLeft(), ldr.getTop() + corner, corner);
		ctx.lineTo(ldr.getLeft(), ldr.getBottom() - corner);
		ctx.arcTo(ldr.getLeft(), ldr.getBottom(), ldr.getLeft() + corner, ldr.getBottom(), corner);
		ctx.lineTo(ldr.getRight() - corner, ldr.getBottom());
		ctx.arcTo(ldr.getRight(), ldr.getBottom(), ldr.getRight(), ldr.getBottom() - corner, corner);
		ctx.lineTo(ldr.getRight(), ldr.getTop() + corner);
		ctx.arcTo(ldr.getRight(), ldr.getTop(), ldr.getRight() + corner, ldr.getTop(), corner);

		ctx.lineTo(r.getRight() - corner, r.getTop());
		ctx.arcTo(r.getRight(), r.getTop(), r.getRight(), r.getTop() + corner, corner);

		ctx.lineTo(r.getRight(), gbr.getTop() - corner);
		ctx.arcTo(r.getRight(), gbr.getTop(), r.getRight() - corner, gbr.getTop(), corner);
		ctx.lineTo(gbr.getLeft() + corner, gbr.getTop());
		ctx.arcTo(gbr.getLeft(), gbr.getTop(), gbr.getLeft(), gbr.getTop() + corner, corner);
		ctx.lineTo(gbr.getLeft(), gbr.getBottom() - corner);
		ctx.arcTo(gbr.getLeft(), gbr.getBottom(), gbr.getLeft() - corner, gbr.getBottom(), corner);
		ctx.lineTo(r.getLeft() + corner, r.getBottom());
		ctx.arcTo(r.getLeft(), r.getBottom(), r.getLeft(), r.getBottom() - corner, corner);
		ctx.lineTo(r.getLeft(), r.getTop() + corner);
		ctx.arcTo(r.getLeft(), r.getTop(), r.getLeft() + corner, r.getTop(), corner);
		ctx.closePath();

		ctx.setLineWidth(Math.ceil(w) * 2);
		ctx.setStrokeStyle(RGB.black().toString());
		ctx.stroke();

		ctx.setLineWidth(Math.ceil(w));
		ctx.setStrokeStyle(EditBufferPresenterProvider.getPresenter().voiceGroupIndicationColor.toString());
		ctx.stroke();
	}

	private void drawBackground(Context2d ctx) {
		Rect r = belt.getPixRect();

		double corner = Millimeter.toPixels(1);
		double activeButtonLeft = buttons.getLeftOfActiveButton();
		double activeButtonRight = buttons.getRightOfActiveButton();
		double activeButtonTop = buttons.getPixRect().getTop();

		ctx.beginPath();
		ctx.moveTo(r.getLeft(), r.getBottom());
		ctx.lineTo(r.getLeft(), r.getTop() + corner);
		ctx.arcTo(r.getLeft(), r.getTop(), r.getLeft() + corner, r.getTop(), corner);

		if (belt.isHidden()) {
			ctx.lineTo(r.getRight() - corner, r.getTop());
			ctx.arcTo(r.getRight(), r.getTop(), r.getRight(), r.getTop() + corner, corner);

		} else {

			ctx.lineTo(activeButtonLeft - corner, r.getTop());
			ctx.arcTo(activeButtonLeft, r.getTop(), activeButtonLeft, r.getTop() - corner, corner);
			ctx.lineTo(activeButtonLeft, activeButtonTop + corner);
			ctx.arcTo(activeButtonLeft, activeButtonTop, activeButtonLeft + corner, activeButtonTop, corner);
			ctx.lineTo(activeButtonRight - corner, activeButtonTop);
			ctx.arcTo(activeButtonRight, activeButtonTop, activeButtonRight, activeButtonTop + corner, corner);
			ctx.lineTo(activeButtonRight, r.getTop() - corner);
			ctx.arcTo(activeButtonRight, r.getTop(), activeButtonRight + corner, r.getTop(), corner);

			if (activeButtonRight != r.getRight()) {
				ctx.lineTo(r.getRight() - corner, r.getTop());
				ctx.arcTo(r.getRight(), r.getTop(), r.getRight(), r.getTop() + corner, corner);
			}
		}

		ctx.lineTo(r.getRight(), r.getBottom());
		ctx.lineTo(r.getLeft(), r.getBottom());
		ctx.closePath();
		ctx.setLineWidth(5);
		ctx.setStrokeStyle(new Gray(0).toString());
		ctx.stroke();

		ctx.setFillStyle(getColorBackground().toString());
		ctx.fill();

		ctx.setLineWidth(2);
		ctx.setStrokeStyle(new Gray(78).toString());
		ctx.stroke();
	}

	public Gray getColorBackground() {
		return new Gray(51);
	}

	@Override
	public void doLayout(double x, double y, double w, double h) {
		getRelativePosition().moveTo(0, 0);
		super.doLayout(x, y, w, h);

		double layerDisplayWidth = Millimeter.toPixels(10);
		double layerDisplayHeight = Millimeter.toPixels(10);
		layerDisplay.doLayout((w - layerDisplayWidth) / 2, 0, layerDisplayWidth, layerDisplayHeight);

		double beltHeight = Millimeter.toPixels(40);
		belt.doLayout(0, h - beltHeight, w, beltHeight);
		beltHeight = belt.getRelativePosition().getHeight();

		buttons.doLayout(w, h - beltHeight);

		double undoRedoMargin = Millimeter.toPixels(2.5);
		undoRedo.doLayout(undoRedoMargin, buttons.getRelativePosition().getTop());

		layoutFloatingWindows(undoRedoMargin);
		layoutContextMenus(w, h);

		globalMenu.doLayout(w, h);

		calcPixRect(new Position(0, 0), 1.0);
		initialLayout = false;
	}

	private void layoutFloatingWindows(double initialWindowOffset) {
		int i = 0;

		for (OverlayControl c : getChildren()) {
			if (c instanceof FloatingWindow) {
				i++;
				FloatingWindow wnd = (FloatingWindow) c;
				if (initialLayout)
					wnd.doLayout(i * initialWindowOffset, i * initialWindowOffset);
				else
					wnd.doLayout(wnd.getRelativePosition().getLeft(), wnd.getRelativePosition().getTop());
			}
		}
	}

	protected void layoutContextMenus(double w, double h) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof ContextMenu) {
				ContextMenu m = (ContextMenu) c;
				double menuWidth = m.getDesiredWidth();
				double menuHeight = m.getDesiredHeight();
				double menuX = m.getRelativePosition().getLeft();
				double menuY = m.getRelativePosition().getTop();

				if (menuX + menuWidth > w)
					menuX = w - menuWidth;

				if (menuY + menuHeight > h)
					menuY = h - menuHeight;

				m.doLayout(menuX, menuY, menuWidth, menuHeight);
			}
		}
	}

	@Override
	public RGB getColorFont() {
		return NonMaps.theMaps.getNonLinearWorld().getColorFont();
	}

	@Override
	public RGB getColorObjectContour() {
		return ColorTable.getColorContour();
	}

	@Override
	public RGB getColorIndicator() {
		return ColorTable.getColorIndicator();
	}

	public DragProxy createDragProxy(Control origin) {
		removeDragProxies();
		return addDragProxy(origin);
	}

	public DragProxy addDragProxy(Control origin) {
		return addChild(new DragProxy(this, origin));
	}

	boolean removeDragProxies() {
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				removeChild(c);
				removeDragProxies();
				return true;
			}
		}
		return false;
	}

	public void removeDragProxy(DragProxy c) {
		removeChild(c);
	}

	public void cancelDragging() {
		removeDragProxies();
	}

	public boolean isCurrentlyDragging(Control client) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				DragProxy p = (DragProxy) c;
				if (p.getOrigin() == client) {
					return true;
				}
			}
		}

		return false;
	}

	public boolean isCurrentlyDraggingATypeOf(String className) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				DragProxy p = (DragProxy) c;
				if (p.getOrigin().getClass().getName().equals(className))
					return true;
			}
		}

		return false;
	}

	public com.nonlinearlabs.client.world.overlay.belt.Belt getBelt() {
		return belt;
	}

	public void update(Node settingsNode, Node editBufferNode, Node presetManagerNode, Node deviceInfo, Node undoNode) {
		undo.update(undoNode);
		belt.update(settingsNode, editBufferNode, presetManagerNode);
		undoRedo.update();
	}

	public ContextMenu setContextMenu(Position pt, ContextMenu ctx) {
		boolean isPresetColorTagMenu = ctx instanceof PresetColorTagContextMenu;

		if (!isPresetColorTagMenu) {
			removeExistingContextMenus();
		}

		addChild(ctx);
		ctx.getRelativePosition().moveTo(pt.getX() - getPixRect().getLeft(), pt.getY() - getPixRect().getTop());
		requestLayout();
		return ctx;
	}

	public List<ContextMenu> getContextMenus() {
		List<ContextMenu> ret = new ArrayList<>();
		for (OverlayControl c : getChildren()) {
			if (c instanceof ContextMenu) {
				ContextMenu m = (ContextMenu) c;
				ret.add(m);
			}
		}
		return ret;
	}

	public ContextMenu getContextMenu() {
		for (OverlayControl c : getChildren()) {
			if (c instanceof ContextMenu) {
				ContextMenu m = (ContextMenu) c;
				return m;
			}
		}
		return null;
	}

	public boolean removeExistingContextMenus() {
		boolean any = false;
		for (OverlayControl c : getChildren()) {
			if (c instanceof ContextMenu) {
				ContextMenu m = (ContextMenu) c;
				m.fadeOut();
				any = true;
			}
		}
		return any;
	}

	public void collapseGlobalMenu() {
		for (OverlayControl c : getChildren()) {
			if (c instanceof GlobalMenu) {
				if (((GlobalMenu) c).getState() == GlobalMenu.State.Expanded) {
					((GlobalMenu) c).toggle();
				}
			}
		}
	}

	@Override
	public Control handleGesture(Gesture g) {
		if (g instanceof Down) {
			if (!hitsContextMenu(((Down) g).getPosition())) {
				removeExistingContextMenus();
			}
			if (!hitsGlobalMenu(((Down) g).getPosition())) {
				collapseGlobalMenu();
			}
		}

		return super.handleGesture(g);
	}

	private boolean hitsContextMenu(Position position) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof ContextMenu) {
				if (c.getPixRect().contains(position))
					return true;
			}
		}
		return false;
	}

	private boolean hitsGlobalMenu(Position position) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof GlobalMenu) {
				if (c.getPixRect().contains(position))
					return true;
			}
		}
		return false;
	}

	public FloatingWindow getPresetSearch() {
		return null;
	}

	public UndoTreeWindow getUndoTree() {
		return undo;
	}

	public void pushToTop(FloatingWindow w) {
		ArrayList<OverlayControl> c = getChildren();
		if (c.get(c.size() - 1) != w) {

			invalidate(INVALIDATION_FLAG_UI_CHANGED);
		}

		for (OverlayControl ctrl : getChildren()) {
			if (ctrl instanceof ContextMenu) {
				c.remove(ctrl);
				c.add(ctrl);
				break;
			}
		}

	}

	public void moveDragProxies(double xDiff, double yDiff) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				DragProxy d = (DragProxy) c;
				d.moveProxy(xDiff, yDiff);
			}
		}

	}

	public LinkedList<DragProxy> getDragProxies() {
		LinkedList<DragProxy> proxies = new LinkedList<DragProxy>();
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				DragProxy d = (DragProxy) c;
				proxies.add(d);
			}
		}
		return proxies;
	}

	public DragProxy getDragProxyFor(Control origin) {
		for (OverlayControl c : getChildren()) {
			if (c instanceof DragProxy) {
				DragProxy d = (DragProxy) c;
				if (d.getOrigin() == origin)
					return d;
			}
		}
		return null;
	}

	public void addCompareDialog(CompareDialog d) {
		compareDialogs.add(d);
	}

	public void removeCompareDialog(CompareDialog d) {
		compareDialogs.remove(d);
	}

	public List<CompareDialog> getCompareDialogs() {
		return compareDialogs;
	}

	public void removeModal(ModalDialog modal) {
		if (modal == modalDialog)
			modalDialog = null;
	}
}