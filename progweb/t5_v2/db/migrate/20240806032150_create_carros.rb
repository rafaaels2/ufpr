class CreateCarros < ActiveRecord::Migration[7.1]
  def change
    create_table :carros do |t|
      t.string :crv
      t.string :nome
      t.string :marca
      t.references :pessoa, null: false, foreign_key: true

      t.timestamps
    end
  end
end
