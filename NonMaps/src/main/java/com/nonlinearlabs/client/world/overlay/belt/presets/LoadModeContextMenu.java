package com.nonlinearlabs.client.world.overlay.belt.presets;

import com.nonlinearlabs.client.dataModel.setup.SetupModel.LoadMode;
import com.nonlinearlabs.client.presenters.PresetManagerPresenterProvider;
import com.nonlinearlabs.client.useCases.EditBufferUseCases;
import com.nonlinearlabs.client.world.Control;
import com.nonlinearlabs.client.world.Position;
import com.nonlinearlabs.client.world.overlay.ContextMenu;
import com.nonlinearlabs.client.world.overlay.ContextMenuItem;
import com.nonlinearlabs.client.world.overlay.OverlayLayout;

public class LoadModeContextMenu extends ContextMenu {

    public LoadModeContextMenu(OverlayLayout parent) {
        super(parent);

        String[] entries = PresetManagerPresenterProvider.get().getPresenter().loadModeMenuEntries;

        addChild(new ContextMenuItem(this, entries[0]) {
            @Override
            public Control click(Position eventPoint) {
                EditBufferUseCases.get().setLoadMode(LoadMode.LoadToPart);
                return super.click(eventPoint);
            }
        });

        addChild(new ContextMenuItem(this, entries[1]) {
            @Override
            public Control click(Position eventPoint) {
                EditBufferUseCases.get().setLoadMode(LoadMode.Select);
                return super.click(eventPoint);
            }
        });

        addChild(new ContextMenuItem(this, entries[2]) {
            @Override
            public Control click(Position eventPoint) {
                EditBufferUseCases.get().setLoadMode(LoadMode.DirectLoad);
                return super.click(eventPoint);
            }
        });
    }

}
