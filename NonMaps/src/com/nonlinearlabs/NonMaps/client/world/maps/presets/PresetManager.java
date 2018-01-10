package com.nonlinearlabs.NonMaps.client.world.maps.presets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;

import com.google.gwt.event.dom.client.KeyDownEvent;
import com.google.gwt.i18n.client.NumberFormat;
import com.google.gwt.xml.client.Document;
import com.google.gwt.xml.client.Node;
import com.google.gwt.xml.client.NodeList;
import com.google.gwt.xml.client.XMLParser;
import com.nonlinearlabs.NonMaps.client.Millimeter;
import com.nonlinearlabs.NonMaps.client.NonMaps;
import com.nonlinearlabs.NonMaps.client.Renameable;
import com.nonlinearlabs.NonMaps.client.ServerProxy;
import com.nonlinearlabs.NonMaps.client.Tracer;
import com.nonlinearlabs.NonMaps.client.world.Control;
import com.nonlinearlabs.NonMaps.client.world.IPreset;
import com.nonlinearlabs.NonMaps.client.world.NonLinearWorld;
import com.nonlinearlabs.NonMaps.client.world.Position;
import com.nonlinearlabs.NonMaps.client.world.Rect;
import com.nonlinearlabs.NonMaps.client.world.maps.MapsControl;
import com.nonlinearlabs.NonMaps.client.world.maps.MapsLayout;
import com.nonlinearlabs.NonMaps.client.world.maps.NonDimension;
import com.nonlinearlabs.NonMaps.client.world.maps.NonPosition;
import com.nonlinearlabs.NonMaps.client.world.maps.NonRect;
import com.nonlinearlabs.NonMaps.client.world.maps.parameters.Parameter.Initiator;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.Bank;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.Updater;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.preset.Preset;
import com.nonlinearlabs.NonMaps.client.world.overlay.BankInfoDialog;
import com.nonlinearlabs.NonMaps.client.world.overlay.DragProxy;
import com.nonlinearlabs.NonMaps.client.world.overlay.PresetInfoDialog;
import com.nonlinearlabs.NonMaps.client.world.overlay.SearchQueryDialog;
import com.nonlinearlabs.NonMaps.client.world.overlay.belt.EditBufferDraggingButton;

public class PresetManager extends MapsLayout {

	public enum SearchQueryCombination {
		AND, OR
	}

	private String selectedBank;
	private String query = "";
	private SearchQueryCombination combination = SearchQueryCombination.OR;
	private Preset currentFilterMatch = null;
	private int currentFileVersion = 0;
	private MultiplePresetSelection multiSelection;
	private MoveAllBanksLayer moveAllBanks;

	public PresetManager(NonLinearWorld parent) {
		super(parent);
	}

	@Override
	public NonLinearWorld getParent() {
		return (NonLinearWorld) super.getParent();
	}

	@Override
	public double getLevelOfDetailForFullVisibility() {
		return 1;
	}
	
	private NonRect getAllBanksOutline() {
		NonRect fence = new NonRect(Double.MAX_VALUE / 2, Double.MAX_VALUE / 2, -Double.MAX_VALUE, -Double.MAX_VALUE);

		for (MapsControl c : getChildren()) {
			if (c instanceof Bank) {
				NonRect banksRect = c.getNonPosition();
				double right = fence.getRight();
				double bottom = fence.getBottom();
				fence.setLeft(Math.min(banksRect.getLeft(), fence.getLeft()));
				fence.setTop(Math.min(banksRect.getTop(), fence.getTop()));
				fence.setBottom(Math.max(banksRect.getBottom(), bottom));
				fence.setRight(Math.max(banksRect.getRight(), right));
			}
		}
		return fence;
	}

	@Override
	public void calcPixRect(Position parentsReference, double currentZoom) {
		if (super.calcPixRectWithoutMargins(parentsReference, currentZoom))
			children.calcPixRect(this.getPixRect().getCenterPoint(), currentZoom);
	}

	public void update(Node presetManagerNode) {

		if (ServerProxy.didChange(presetManagerNode)) {
			readPlaygroundFileVersion(presetManagerNode);

			Preset oldPresetSelection = getSelectedPreset();
			NodeList children = presetManagerNode.getChildNodes();

			for (int i = 0; i < children.getLength(); i++) {
				Node child = children.item(i);

				if (child.getNodeName().equals("banks"))
					updateBanks(child);
			}

			refreshFilter(false);

			Preset newPresetSelection = getSelectedPreset();

			if (oldPresetSelection != newPresetSelection) {
				onPresetSelectionChanged(newPresetSelection);
			}

		}
	}

	private void readPlaygroundFileVersion(Node presetManagerNode) {
		Node fileVersion = presetManagerNode.getAttributes().getNamedItem("file-version");
		if (fileVersion != null)
			currentFileVersion = Integer.parseInt(fileVersion.getNodeValue());
	}

	public int getCurrentFileVersion() {
		return currentFileVersion;
	}

	public void onPresetSelectionChanged(Preset newPresetSelection) {
		if (PresetInfoDialog.isShown())
			PresetInfoDialog.update(newPresetSelection);

		if (NonMaps.theMaps.getNonLinearWorld().getSettings().isOneOf("SelectionAutoScroll", "on", "preset", "parameter-and-preset"))
			scrollToSelectedPreset();
	}

	private void scrollToSelectedPreset() {
		Rect pixRect = getNonMaps().getNonLinearWorld().getViewport().getPixRectWithoutBelt();
		Preset p = getSelectedPreset();

		if (p != null) {

			if (pixRect.contains(p.getPixRect())) {
				return;
			}

			Bank b = p.getParent();

			if (!pixRect.contains(b.getPixRect()))
				b.scrollToMakeFullyVisible();
		}
	}

	private void updateBanks(Node banks) {
		clearDockingRelations();

		ArrayList<MapsControl> currentChildren = new ArrayList<MapsControl>();

		for (MapsControl c : getChildren()) {
			if (c instanceof Bank)
				currentChildren.add((Bank) c);
		}

		selectedBank = banks.getAttributes().getNamedItem("selected-bank").getNodeValue();
		NodeList bankList = banks.getChildNodes();

		for (int i = 0; i < bankList.getLength(); i++) {
			Node bank = bankList.item(i);
			if (bank.getNodeName().equals("preset-bank")) {
				Renameable ui = updateBank(bank);
				currentChildren.remove(ui);
			}
		}

		removeObsoleteBanks(currentChildren);
		installDockingRelations();
		requestLayout();

		if (BankInfoDialog.isShown())
			BankInfoDialog.update();
	}

	private void clearDockingRelations() {
		for (MapsControl control : getChildren()) {
			if (control instanceof Bank) {
				Bank bank = (Bank) control;
				bank.removeAllSlaves();
				bank.removeMaster();
			}
		}
	}

	private void installDockingRelations() {
		for (MapsControl control : getChildren()) {
			if (control instanceof Bank) {
				Bank bank = (Bank) control;
				Bank masterBank = NonMaps.theMaps.getNonLinearWorld().getPresetManager().findBank(bank.getMasterUUID());

				if(masterBank != null)
				{
					bank.installRelationshipMasterSlave(masterBank, bank.getAttatchDirection());
				}
			}
		}
	}

	public void toggleHiddenBanks() {
		setVisible(!isIntendedVisible());
	}

	private void removeObsoleteBanks(ArrayList<MapsControl> currentChildren) {
		boolean needsLayout = false;

		for (MapsControl control : currentChildren) {
			removeChild(control);
			needsLayout = true;
		}

		if (needsLayout)
			requestLayout();
	}

	private Renameable updateBank(Node bank) {
		String uuid = bank.getAttributes().getNamedItem("uuid").getNodeValue();
		Bank bankUI = findBank(uuid);

		if (bankUI == null) {
			bankUI = addChild(new Bank(this, uuid));
			Updater updater = new Updater(bankUI);
			updater.update(bank, true);
			requestLayout();
		} else if (ServerProxy.didChange(bank)) {
			Updater updater = new Updater(bankUI);
			updater.update(bank, false);
		}
		return bankUI;
	}

	public Bank findBank(String uuid) {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				if (b.getUUID().equals(uuid))
					return b;
			}
		}
		return null;
	}

	public String getSelectedBank() {
		return selectedBank;
	}

	public void pushBankOntoTop(Bank presetBank) {
		getChildren().remove(presetBank);
		addChild(presetBank);
	}

	@Override
	public <T extends MapsControl> T addChild(T child) {
		T ret = super.addChild(child);

		if (moveAllBanks != null) {
			getChildren().remove(moveAllBanks);
			super.addChild(moveAllBanks);
		}

		return ret;
	}

	private void highlightSlavesOfSelectedBank(Bank master)
	{
		Bank rightSlave = master.getRightSlave();
		Bank bottomSlave = master.getBottomSlave();
		
		if(rightSlave != null )
		{
			rightSlave.setInSelectedCluster(true);
			highlightSlavesOfSelectedBank(rightSlave);
		}
		if(bottomSlave != null)
		{
			bottomSlave.setInSelectedCluster(true);
			highlightSlavesOfSelectedBank(bottomSlave);
		}
	}
	
	public void resetClusterHighlight()
	{
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				b.setInSelectedCluster(false);
			}
		}
	}
	
	public void indicateClusterSelection(String bankUUID)
	{
		resetClusterHighlight();
		Bank bankObj = findBank(bankUUID);

		if(bankObj != null)
		{
			bankObj.setInSelectedCluster(true);
			if(bankObj.getBottomSlave() != null || bankObj.getRightSlave() != null)
			{
				highlightSlavesOfSelectedBank(bankObj);
			}
		}
	}
	
	public void selectBank(String bankUUID, boolean userInteraction) {
		if (!selectedBank.equals(bankUUID)) {
			selectedBank = bankUUID;
			invalidate(INVALIDATION_FLAG_UI_CHANGED);
			
			if (userInteraction)
				getNonMaps().getServerProxy().selectBank(bankUUID);

			if (PresetInfoDialog.isShown())
				PresetInfoDialog.update();

			if (BankInfoDialog.isShown())
				BankInfoDialog.update();
		}
	}

	@Override
	public void doFirstLayoutPass(double levelOfDetail) {
		for (MapsControl c : getChildren())
			c.doFirstLayoutPass(levelOfDetail);

		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				if (b.getMaster() == null) {
					b.layoutSlaves();
				}
			}
		}
	}

	@Override
	public void doSecondLayoutPass(double parentsWidthFromFirstPass, double parentsHeightFromFirstPass) {
		setNonSize(parentsWidthFromFirstPass, parentsHeightFromFirstPass);
		getNonPosition().moveTo(-parentsWidthFromFirstPass / 2, -parentsHeightFromFirstPass / 2);

		if (moveAllBanks != null)
			moveAllBanks.getNonPosition().set(getAllBanksOutline());
	}

	@Override
	public Control drag(Position pos, DragProxy dragProxy) {
		if (dragProxy.getOrigin() instanceof Bank) {
			Bank dragged = (Bank) dragProxy.getOrigin();	
			boolean shouldBeOn = indicateAttachTargets(dragged, dragProxy.getPixRect()) != null;
			dragged.setInSelectedCluster(shouldBeOn || dragged.hasSlaves());
			return this;
		}
		
		if (dragProxy.getOrigin() instanceof IPreset) {
			if (!getNonMaps().getNonLinearWorld().getParameterEditor().getPixRect().contains(pos))
				return this;
		}

		if (dragProxy.getOrigin() instanceof EditBufferDraggingButton)
			return this;

		return super.drag(pos, dragProxy);
	}

	private boolean isAttachTargetToRight(Bank targetBank, Rect dragProxie)
	{
		Rect rightDockingRect = targetBank.getPixRect().copy();
		rightDockingRect.set(rightDockingRect.getLeft() + rightDockingRect.getWidth() / 2, 
				rightDockingRect.getTop() - getAttachArea() / 2, 
				rightDockingRect.getWidth(), 
				rightDockingRect.getHeight());
		
		if(rightDockingRect.contains(dragProxie.getLeftTop()))
		{
			return true;
		}
		return false;
	}
	
	private boolean isAttachTargetToBottom(Bank targetBank, Rect dragProxie)
	{
		Rect bottomDockingRect = targetBank.getPixRect().copy();
		bottomDockingRect.set(bottomDockingRect.getLeft(), 
				bottomDockingRect.getBottom() - getAttachArea(), 
				bottomDockingRect.getWidth() / 2, 
				getAttachArea() * 2);
		
		
		if(bottomDockingRect.contains(dragProxie.getLeftTop()))
		{
			return true;
		}
		return false;
	}
	
	private Bank indicateAttachTargets(Bank draggedBank, Rect dragProxie) {
		Bank ret = null;
		
		for (Control control : getChildren()) {
			if (control instanceof Bank) {
				Bank targetBank = (Bank) control;
				
				if (targetBank != draggedBank && isAttachTargetToRight(targetBank, dragProxie)) {
					if(targetBank.setIsDockingTarget("right"));
						ret = targetBank;

				} else if (targetBank != draggedBank && isAttachTargetToBottom(targetBank, dragProxie)) {
					if(targetBank.setIsDockingTarget("bottom"));
						ret = targetBank;
				} else {
					targetBank.unsetIsDockingTarget();
				}
			}
		}
		
		return ret;
	}

	private double getAttachArea() {
		return Millimeter.toPixels(10);
	}

	@Override
	public void dragLeave() {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				b.unsetIsDockingTarget();
			}
		}
	}

	@Override
	public Control drop(Position pos, DragProxy dragProxy) {
		
		NonMaps.theMaps.getNonLinearWorld().getPresetManager().resetClusterHighlight();
		
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				if (b.isAttachDropTarget()) {
					Control ret = b.drop(pos, dragProxy);
					dragLeave();
					return ret;
				}
			}
		}

		if (dragProxy.getOrigin() instanceof Bank) {
			
			Bank b = (Bank) dragProxy.getOrigin();

			if (b.getMaster() != null)
				b.undockBank();

			NonPosition np = toNonPosition(dragProxy.getPixRect().getPosition());
			np.snapTo(getSnapGridResolution());
			b.getNonPosition().moveTo(np);
			b.requestLayout();
			getNonMaps().getServerProxy().onBankPositionChanged(b);
			
			return this;
		} else if (dragProxy.getOrigin() instanceof IPreset) {

			IPreset p = (IPreset) dragProxy.getOrigin();

			if (p instanceof Preset) {
				Preset preset = (Preset) p;
				Rect intersection = dragProxy.getPixRect().getIntersection(preset.getParent().getPixRect());
				if (!intersection.isEmpty(0)) {
					return this;
				}
			}

			if (hasMultiplePresetSelection()) {
				getNonMaps().getServerProxy().createNewBankFromPresets(toNonPosition(pos), getMultiSelection().getCSV());

			} else {
				getNonMaps().getServerProxy().createNewBankFromPreset(toNonPosition(pos), p);
			}

			return this;
		} else if (dragProxy.getOrigin() instanceof EditBufferDraggingButton) {
			getNonMaps().getServerProxy().newBankFromEditBuffer(toNonPosition(pos));
		}
		return null;
	}

	public boolean canNext() {
		Preset p = findLoadedPreset();
		if (p != null) {
			Bank b = p.getParent();
			if (b != null)
				return !b.isLast(p);
		}
		return false;
	}

	public boolean canPrev() {
		Preset p = findLoadedPreset();
		if (p != null) {
			Bank b = p.getParent();
			if (b != null)
				return !b.isFirst(p);
		}
		return false;
	}

	public Preset findLoadedPreset() {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				Preset p = b.findLoadedPreset();
				if (p != null)
					return p;
			}
		}
		return null;
	}

	@Override
	public Control onKey(final KeyDownEvent event) {

		int keyCode = event.getNativeEvent().getKeyCode();

		if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_RIGHT) {
			selectNextBank(true);
			return this;
		} else if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_LEFT) {
			selectPreviousBank(true);
			return this;
		} else if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_DOWN) {
			selectNextPreset(Initiator.EXPLICIT_USER_ACTION);
			return this;
		} else if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_UP) {
			selectPreviousPreset(Initiator.EXPLICIT_USER_ACTION);
			return this;
		} else if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_ENTER) {
			loadSelectedPreset();
			return this;
		} else if (keyCode == com.google.gwt.event.dom.client.KeyCodes.KEY_H) {
			toggleHiddenBanks();
			return this;
		}
		return null;
	}

	public void selectPreviousPreset(Initiator initiator) {
		Bank b = findBank(getSelectedBank());
		if (b != null)
			b.selectPrev(initiator);
	}

	public void selectNextPreset(Initiator initiator) {
		Bank b = findBank(getSelectedBank());
		if (b != null)
			b.selectNext(initiator);
	}

	public boolean hasSelectedBank() {
		return findBank(getSelectedBank()) != null;
	}

	public boolean hasSelectedPreset() {
		Bank b = findBank(getSelectedBank());

		if (b != null)
			return b.hasSelectedPreset();

		return false;
	}

	public void selectPreviousBank(boolean userInteraction) {
		selectBankWithOrdernumberOffset(-1);
	}

	public void selectNextBank(boolean userInteraction) {
		selectBankWithOrdernumberOffset(1);
	}

	public boolean canSelectPreviousBank() {
		String sel = getSelectedBank();
		Bank b = findBank(sel);
		return canSelectBankWithOrdernumberOffset(b, -1);
	}

	public boolean canSelectNextBank() {
		String sel = getSelectedBank();
		Bank b = findBank(sel);
		return canSelectBankWithOrdernumberOffset(b, 1);
	}

	public boolean canSelectBankWithOrdernumberOffset(Bank b, int i) {
		if (b != null) {
			int targetOrderNumber = b.getOrderNumber() + i;

			for (Control c : getChildren()) {
				if (c instanceof Bank) {
					Bank candidate = (Bank) c;
					if (candidate.getOrderNumber() == targetOrderNumber)
						return true;
				}
			}
		}
		return false;
	}

	protected void selectBankWithOrdernumberOffset(int off) {
		String sel = getSelectedBank();
		Bank b = findBank(sel);

		if (b == null)
			selectBankWithOrderNumber(1);
		else
			selectBankWithOrderNumber(b.getOrderNumber() + off);
	}

	private void selectBankWithOrderNumber(int i) {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				if (b.getOrderNumber() == i) {
					b.selectBank(true);
					return;
				}
			}
		}
	}

	public void loadSelectedPreset() {
		Preset p = findSelectedPreset();

		if (p != null)
			getNonMaps().getServerProxy().loadPreset(p);
	}

	public Preset findSelectedPreset() {
		Bank b = findBank(getSelectedBank());
		if (b != null)
			return b.findPreset(b.getSelectedPreset());

		return null;
	}

	public void setFilter(String query) {
		if (!this.query.equals(query)) {
			Tracer.log("PresetManager.setFilter " + query);
			this.query = query;
			refreshFilter(true);
		}
	}

	public void setSearchQueryCombination(SearchQueryCombination c) {
		if (combination != c) {
			this.combination = c;
			refreshFilter(true);
		}
	}

	private void refreshFilter(final boolean autoZoom) {
		if (this.query.isEmpty()) {
			clearFilter();
		} else {
			NonMaps.theMaps.getServerProxy().searchPresets(query, combination, new ServerProxy.DownloadHandler() {

				@Override
				public void onFileDownloaded(String text) {
					Document xml = XMLParser.parse(text);
					Node presetManager = xml.getElementsByTagName("preset-manager").item(0);
					int numMatches = applyFilter(presetManager, autoZoom);
					SearchQueryDialog.setMatches(numMatches);
				}

				@Override
				public void onError() {
				}
			});
		}

		if (currentFilterMatch != null)
			if (currentFilterMatch.getFilterState() != Preset.FilterState.FILTER_MATCHES)
				currentFilterMatch = null;
	}

	public void clearFilter() {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				b.clearFilter();
			}
		}

		query = "";
		currentFilterMatch = null;
		SearchQueryDialog.setMatches(0);
	}

	private int applyFilter(Node presetManager, boolean autoZoom) {
		int numMatches = 0;
		Node banksNode = findBanksNode(presetManager);
		NodeList banks = banksNode.getChildNodes();

		for (int i = 0; i < banks.getLength(); i++) {
			Node child = banks.item(i);

			if (child.getNodeName().equals("preset-bank"))
				numMatches += applyBankFilter(child);
		}

		if (autoZoom)
			zoomToAllFilterMatches();

		return numMatches;
	}

	private int applyBankFilter(Node bank) {
		String uuid = bank.getAttributes().getNamedItem("uuid").getNodeValue();
		Bank bankUI = findBank(uuid);
		return bankUI.applyFilter(bank);
	}

	private Node findBanksNode(Node presetManager) {
		NodeList banks = presetManager.getChildNodes();

		for (int i = 0; i < banks.getLength(); i++) {
			Node child = banks.item(i);

			if (child.getNodeName().equals("banks"))
				return child;
		}

		return null;
	}

	public LinkedList<Preset> collectMatchingPresets() {
		LinkedList<Preset> ret = new LinkedList<Preset>();

		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;

				for (Control f : b.getChildren()) {
					if (f instanceof Preset) {
						Preset p = (Preset) f;

						if (p.getFilterState() == Preset.FilterState.FILTER_MATCHES) {
							ret.add(p);
						}
					}
				}
			}
		}
		return ret;
	}

	private void setCurrentFilterMatch(Preset p) {
		if (currentFilterMatch != p) {
			currentFilterMatch = p;
			invalidate(INVALIDATION_FLAG_UI_CHANGED);
		}
	}

	public boolean isCurrentFilterMatch(Preset p) {
		return (currentFilterMatch == p);
	}

	public void highlightNextFilterMatch() {
		LinkedList<Preset> matches = collectMatchingPresets();
		highlightNextInList(matches);
	}

	private void highlightNextInList(LinkedList<Preset> matches) {
		for (Preset p : matches) {
			if (currentFilterMatch == null) {
				setCurrentFilterMatch(p);
				break;
			} else if (p == currentFilterMatch) {
				currentFilterMatch = null;
			}
		}

		if (currentFilterMatch == null && !matches.isEmpty()) {
			setCurrentFilterMatch(matches.getFirst());
		}
	}

	public void highlightPreviousFilterMatch() {
		LinkedList<Preset> matches = collectMatchingPresets();
		Collections.reverse(matches);
		highlightNextInList(matches);
	}

	public void zoomToAllFilterMatches() {
		Boolean setting = Boolean.parseBoolean(NonMaps.theMaps.getNonLinearWorld().getSettings().get("search-auto-zoom", "false"));

		if (setting) {
			currentFilterMatch = null;

			double minX = Double.MAX_VALUE;
			double minY = Double.MAX_VALUE;
			double maxX = -Double.MAX_VALUE;
			double maxY = -Double.MAX_VALUE;
			double presetsHeight = 0;

			for (Preset p : collectMatchingPresets()) {
				minX = Math.min(minX, p.getPixRect().getLeft());
				maxX = Math.max(maxX, p.getPixRect().getRight());
				minY = Math.min(minY, p.getPixRect().getTop());
				maxY = Math.max(maxY, p.getPixRect().getBottom());
				presetsHeight = p.getNonPosition().getHeight();
			}

			if (minX != Double.MAX_VALUE && minY != Double.MAX_VALUE && maxX != Double.MAX_VALUE && maxY != Double.MAX_VALUE) {
				NonPosition leftTop = toNonPosition(new Position(minX, minY));
				NonPosition rightBottom = toNonPosition(new Position(maxX, maxY));
				NonDimension dim = new NonDimension(rightBottom.getX() - leftTop.getX(), rightBottom.getY() - leftTop.getY());
				NonRect r = new NonRect(leftTop, dim);

				double minHeight = 7 * presetsHeight;

				if (dim.getHeight() < minHeight)
					r.enlargeToHeight(minHeight);

				NonMaps.theMaps.getNonLinearWorld().zoomTo(r, true);
			}
		}
	}

	public boolean isEmpty() {
		for (Control c : getChildren())
			if (c instanceof Bank)
				if (!((Bank) c).isEmpty())
					return false;

		return true;
	}

	public String getLoadedPresetNumberString() {
		Preset p = findLoadedPreset();

		if (p != null) {
			Bank b = p.getParent();

			if (b != null) {
				NumberFormat f = NumberFormat.getFormat("000");
				String ret = b.getOrderNumber() + "-" + f.format(p.getNumber());

				if (NonMaps.theMaps.getNonLinearWorld().getParameterEditor().isModified())
					ret += " *";

				return ret;
			}
		}
		return "";
	}

	public Preset getSelectedPreset() {
		return findSelectedPreset();
	}

	public SearchQueryCombination getSearchQueryCombination() {
		return combination;
	}

	public boolean getPresetsVisible() {
		return isVisible();
	}

	public MultiplePresetSelection getMultiSelection() {
		return multiSelection;
	}

	public boolean hasMultiplePresetSelection() {
		return multiSelection != null;
	}

	public MultiplePresetSelection startMultiSelection(Preset p) {
		multiSelection = new MultiplePresetSelection(p);
		return getMultiSelection();
	}

	public void closeMultiSelection() {
		if (hasMultiplePresetSelection()) {
			multiSelection = null;
			requestLayout();
		}
	}

	public Preset findPreset(String uuid) {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				Preset p = b.findPreset(uuid);
				if (p != null)
					return p;
			}
		}

		return null;
	}

	@Override
	public Control click(Position eventPoint) {
		if (moveAllBanks != null) {
			toggleMoveAllBanks();
			return this;
		}

		if (multiSelection != null) {
			closeMultiSelection();
			return this;
		}
		return super.click(eventPoint);
	}

	public void toggleMoveAllBanks() {
		if (moveAllBanks != null) {
			removeChild(moveAllBanks);
			moveAllBanks = null;
		} else {
			super.addChild(moveAllBanks = new MoveAllBanksLayer(this));
		}
		requestLayout();
	}

	public void moveAllBanksBy(NonDimension distance) {
		for (Control c : getChildren()) {
			if (c instanceof Bank) {
				Bank b = (Bank) c;
				NonPosition np = b.getNonPosition().getPosition().copy();
				np.moveBy(distance);
				np.snapTo(getSnapGridResolution());
				b.getNonPosition().moveTo(np);
				getNonMaps().getServerProxy().onBankPositionChanged(b);
			}
		}
		requestLayout();
	}

	public static int getSnapGridResolution() {
		return 15;
	}


}
