class CreateDisPes < ActiveRecord::Migration[7.1]
  def change
    create_table :dis_pes do |t|
      t.references :disciplina, null: false, foreign_key: true
      t.references :pessoa, null: false, foreign_key: true

      t.timestamps
    end
  end
end
