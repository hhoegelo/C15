package com.nonlinearlabs.client.world.maps.parameters.MonoAndUnison;

import com.nonlinearlabs.client.useCases.EditBufferUseCases;
import com.nonlinearlabs.client.world.Rect;
import com.nonlinearlabs.client.world.maps.MapsLayout;
import com.nonlinearlabs.client.world.maps.parameters.BooleanControlSmall;
import com.nonlinearlabs.client.world.maps.parameters.ModulateableKnobWithoutHeader;
import com.nonlinearlabs.client.world.maps.parameters.NumericalControlSmall;
import com.nonlinearlabs.client.world.maps.parameters.Parameter;
import com.nonlinearlabs.client.world.maps.parameters.ParameterColumn;
import com.nonlinearlabs.client.world.maps.parameters.ParameterGroup;
import com.nonlinearlabs.client.world.maps.parameters.UnModulateableParameterName;

public class MonoColumn extends ParameterColumn {
    private final class GlideKnob extends ModulateableKnobWithoutHeader {
        private GlideKnob(MapsLayout parent, int parameterID) {
            super(parent, parameterID);
        }
    }

    private class Enable extends Parameter {

        private Enable(MapsLayout parent) {
            super(parent, 364);
            addChild(new UnModulateableParameterName(this));
            addChild(new BooleanControlSmall(this, getParameterNumber()));
        }

        @Override
        protected int getBackgroundRoundings() {
            return Rect.ROUNDING_NONE;
        }

        @Override
        protected void startMouseEdit() {
            currentParameterChanger = EditBufferUseCases.get().startEditParameterValue(getParameterNumber(),
                    getPixRect().getWidth() / 2);
        }
    }

    private class PriorityControlSmall extends NumericalControlSmall {

        public PriorityControlSmall(MapsLayout parent, int parameterID) {
            super(parent, parameterID);
        }

        @Override
        protected double getInsetY() {
            return 2;
        }

        @Override
        protected double getBasicHeight() {
            return 19;
        }
    }

    private class Priority extends Parameter {
        private Priority(MapsLayout parent) {
            super(parent, 365);
            addChild(new UnModulateableParameterName(this));
            addChild(new PriorityControlSmall(this, getParameterNumber()));
        }
    }

    private class Legato extends Parameter {
        private Legato(MapsLayout parent) {
            super(parent, 366);
            addChild(new UnModulateableParameterName(this));
            addChild(new BooleanControlSmall(this, getParameterNumber()));
        }
    }

    public MonoColumn(ParameterGroup parent) {
        super(parent);
        addChild(new Enable(this));
        addChild(new Priority(this));
        addChild(new GlideKnob(this, 367));
        addChild(new Legato(this));
    }
}
